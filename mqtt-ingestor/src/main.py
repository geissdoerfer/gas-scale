"""
MQTT Ingestor Service - Entry Point
Subscribes to MQTT broker and writes sensor data to database.
"""
import logging
import signal
import sys
from config import Config
from mqtt_client import MQTTIngestor
from db_writer import DatabaseWriter


def setup_logging():
    """Configure logging"""
    logging.basicConfig(
        level=getattr(logging, Config.LOG_LEVEL),
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )


def signal_handler(sig, frame):
    """Handle graceful shutdown"""
    logging.info("Shutting down gracefully...")
    sys.exit(0)


def main():
    """Main entry point"""
    setup_logging()
    logger = logging.getLogger(__name__)

    # Register signal handlers for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    logger.info("=" * 60)
    logger.info("Starting MQTT Ingestor Service")
    logger.info("=" * 60)
    logger.info(f"MQTT Broker: {Config.MQTT_BROKER_HOST}:{Config.MQTT_BROKER_PORT}")
    logger.info(f"Topic: {Config.MQTT_TOPIC}")
    logger.info(f"Database: {Config.POSTGRES_HOST}:{Config.POSTGRES_PORT}/{Config.POSTGRES_DB}")
    logger.info("=" * 60)

    try:
        # Initialize database writer
        logger.info("Initializing database connection pool...")
        db_writer = DatabaseWriter()

        # Initialize MQTT client
        logger.info("Initializing MQTT client...")
        mqtt_client = MQTTIngestor(db_writer)

        # Connect to MQTT broker
        mqtt_client.connect()

        # Start loop (blocking)
        mqtt_client.loop_forever()

    except KeyboardInterrupt:
        logger.info("Received keyboard interrupt, shutting down...")
    except Exception as e:
        logger.error(f"Fatal error: {e}", exc_info=True)
        sys.exit(1)
    finally:
        logger.info("MQTT Ingestor Service stopped")


if __name__ == "__main__":
    main()
