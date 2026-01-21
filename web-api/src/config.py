"""
Configuration management for Web API.
Loads settings from environment variables using Pydantic.
"""
from pydantic_settings import BaseSettings
from typing import List


class Settings(BaseSettings):
    # Database Configuration
    POSTGRES_HOST: str = "postgres"
    POSTGRES_PORT: int = 5432
    POSTGRES_DB: str = "duoclean"
    POSTGRES_USER: str = "duoclean_user"
    POSTGRES_PASSWORD: str = "password"

    # JWT Configuration
    JWT_SECRET: str = "change_this_secret_key"
    JWT_ALGORITHM: str = "HS256"
    JWT_ACCESS_TOKEN_EXPIRE_MINUTES: int = 60
    JWT_REFRESH_TOKEN_EXPIRE_DAYS: int = 7

    # CORS Configuration
    API_CORS_ORIGINS: str = "http://localhost:3000,http://localhost"

    # Logging
    LOG_LEVEL: str = "INFO"

    @property
    def DATABASE_URL(self) -> str:
        """Get PostgreSQL connection URL"""
        return f"postgresql://{self.POSTGRES_USER}:{self.POSTGRES_PASSWORD}@{self.POSTGRES_HOST}:{self.POSTGRES_PORT}/{self.POSTGRES_DB}"

    @property
    def CORS_ORIGINS_LIST(self) -> List[str]:
        """Get CORS origins as list"""
        return [origin.strip() for origin in self.API_CORS_ORIGINS.split(",")]

    class Config:
        env_file = ".env"
        case_sensitive = True


# Create global settings instance
settings = Settings()
