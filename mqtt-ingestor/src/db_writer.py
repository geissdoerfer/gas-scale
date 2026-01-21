"""
Database writer for sensor readings.
Handles connection pooling and data insertion.
"""
import logging
from datetime import datetime
import psycopg2
from psycopg2 import pool
from config import Config

logger = logging.getLogger(__name__)


class DatabaseWriter:
    def __init__(self):
        """Initialize database connection pool"""
        try:
            self.connection_pool = psycopg2.pool.ThreadedConnectionPool(
                Config.DB_POOL_MIN,
                Config.DB_POOL_MAX,
                host=Config.POSTGRES_HOST,
                port=Config.POSTGRES_PORT,
                database=Config.POSTGRES_DB,
                user=Config.POSTGRES_USER,
                password=Config.POSTGRES_PASSWORD
            )
            logger.info("Database connection pool created successfully")
        except Exception as e:
            logger.error(f"Failed to create database connection pool: {e}")
            raise

    def _get_connection(self):
        """Get connection from pool"""
        return self.connection_pool.getconn()

    def _return_connection(self, conn):
        """Return connection to pool"""
        self.connection_pool.putconn(conn)

    def _ensure_device_exists(self, conn, device_id: str):
        """
        Ensure device exists in database (auto-register if not).

        Args:
            conn: Database connection
            device_id: Device identifier
        """
        with conn.cursor() as cursor:
            # Check if device exists
            cursor.execute(
                "SELECT id FROM devices WHERE device_id = %s",
                (device_id,)
            )
            result = cursor.fetchone()

            if result is None:
                # Device doesn't exist, insert it
                cursor.execute(
                    """
                    INSERT INTO devices (device_id, name, description)
                    VALUES (%s, %s, %s)
                    ON CONFLICT (device_id) DO NOTHING
                    """,
                    (device_id, f"Device {device_id}", "Auto-registered device")
                )
                conn.commit()
                logger.info(f"Auto-registered new device: {device_id}")

    def insert_reading(
        self,
        device_id: str,
        timestamp: datetime,
        load: float = None,
        battery_voltage: float = None,
        temperature: float = None
    ):
        """
        Insert sensor reading into database.

        Args:
            device_id: Device identifier
            timestamp: Reading timestamp
            load: Load reading (optional)
            battery_voltage: Battery voltage reading (optional)
            temperature: Temperature reading (optional)
        """
        conn = None
        try:
            conn = self._get_connection()

            # Ensure device exists
            self._ensure_device_exists(conn, device_id)

            # Insert reading (idempotent with ON CONFLICT)
            with conn.cursor() as cursor:
                cursor.execute(
                    """
                    INSERT INTO sensor_readings (time, device_id, load, battery_voltage, temperature)
                    VALUES (%s, %s, %s, %s, %s)
                    ON CONFLICT (time, device_id) DO NOTHING
                    """,
                    (timestamp, device_id, load, battery_voltage, temperature)
                )
                conn.commit()
                logger.debug(f"Inserted reading for device {device_id} at {timestamp}")

        except Exception as e:
            if conn:
                conn.rollback()
            logger.error(f"Database error while inserting reading: {e}")
            raise
        finally:
            if conn:
                self._return_connection(conn)

    def close(self):
        """Close all database connections"""
        if self.connection_pool:
            self.connection_pool.closeall()
            logger.info("Database connections closed")
