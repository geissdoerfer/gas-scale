"""
Database writer for sensor readings.
Handles connection pooling and data insertion.
"""
import logging
from datetime import datetime
import psycopg
from psycopg_pool import ConnectionPool
from config import Config

logger = logging.getLogger(__name__)


class DatabaseWriter:
    def __init__(self):
        """Initialize database connection pool"""
        try:
            # Build connection string for psycopg3
            conninfo = (
                f"host={Config.POSTGRES_HOST} "
                f"port={Config.POSTGRES_PORT} "
                f"dbname={Config.POSTGRES_DB} "
                f"user={Config.POSTGRES_USER} "
                f"password={Config.POSTGRES_PASSWORD}"
            )

            self.connection_pool = ConnectionPool(
                conninfo,
                min_size=Config.DB_POOL_MIN,
                max_size=Config.DB_POOL_MAX
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

    def _device_exists(self, conn, device_id: str) -> bool:
        """
        Check whether a device has been provisioned. Devices are no longer auto-registered
        on first message - they must be created ahead of time (see ../../register_device.py
        or the admin UI's "Create Device" form), which is also what generates the device's
        pairing_secret for the Android app.

        Args:
            conn: Database connection
            device_id: Device identifier

        Returns:
            True if the device exists
        """
        with conn.cursor() as cursor:
            cursor.execute(
                "SELECT 1 FROM devices WHERE device_id = %s",
                (device_id,)
            )
            return cursor.fetchone() is not None

    def insert_reading(
        self,
        device_id: str,
        timestamp: datetime,
        raw_value: float = None,
        battery_voltage: float = None,
        temperature: float = None
    ):
        """
        Insert sensor reading into database.

        Args:
            device_id: Device identifier
            timestamp: Reading timestamp
            raw_value: Raw ADC value from HX711 sensor (optional)
            battery_voltage: Battery voltage reading (optional)
            temperature: Temperature reading (optional)
        """
        conn = None
        try:
            conn = self._get_connection()

            # Reject readings from devices that haven't been provisioned yet.
            if not self._device_exists(conn, device_id):
                logger.warning(
                    f"Rejecting reading for unregistered device '{device_id}' - "
                    "provision it first (register_device.py or the admin UI)"
                )
                return

            # Insert reading (idempotent with ON CONFLICT)
            with conn.cursor() as cursor:
                cursor.execute(
                    """
                    INSERT INTO sensor_readings (time, device_id, raw_value, battery_voltage, temperature)
                    VALUES (%s, %s, %s, %s, %s)
                    ON CONFLICT (time, device_id) DO NOTHING
                    """,
                    (timestamp, device_id, raw_value, battery_voltage, temperature)
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
