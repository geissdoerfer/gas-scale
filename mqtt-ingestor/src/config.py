"""
Configuration management for MQTT Ingestor.
Loads settings from environment variables.
"""
import os


class Config:
    # MQTT Configuration
    MQTT_BROKER_HOST = os.getenv('MQTT_BROKER_HOST', 'localhost')
    MQTT_BROKER_PORT = int(os.getenv('MQTT_BROKER_PORT', 1883))
    MQTT_TOPIC = os.getenv('MQTT_TOPIC', 'sensors/+/data')
    MQTT_CLIENT_ID = os.getenv('MQTT_CLIENT_ID', 'duoclean-ingestor')
    MQTT_QOS = int(os.getenv('MQTT_QOS', 1))
    MQTT_KEEPALIVE = int(os.getenv('MQTT_KEEPALIVE', 60))
    MQTT_USERNAME = os.getenv('MQTT_USERNAME')
    MQTT_PASSWORD = os.getenv('MQTT_PASSWORD')

    # Database Configuration
    POSTGRES_HOST = os.getenv('POSTGRES_HOST', 'postgres')
    POSTGRES_PORT = int(os.getenv('POSTGRES_PORT', 5432))
    POSTGRES_DB = os.getenv('POSTGRES_DB', 'duoclean')
    POSTGRES_USER = os.getenv('POSTGRES_USER', 'duoclean_user')
    POSTGRES_PASSWORD = os.getenv('POSTGRES_PASSWORD', 'password')

    DB_POOL_MIN = int(os.getenv('DB_POOL_MIN', 2))
    DB_POOL_MAX = int(os.getenv('DB_POOL_MAX', 10))

    # Logging
    LOG_LEVEL = os.getenv('LOG_LEVEL', 'INFO')

    @classmethod
    def get_db_url(cls):
        """Get PostgreSQL connection URL"""
        return f"postgresql://{cls.POSTGRES_USER}:{cls.POSTGRES_PASSWORD}@{cls.POSTGRES_HOST}:{cls.POSTGRES_PORT}/{cls.POSTGRES_DB}"
