"""
FastAPI application entry point for DuoClean Energy Web API.
"""
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime
import logging

from src.config import settings
from src.routers import auth, users, devices, readings
from src import schemas

# Configure logging
logging.basicConfig(
    level=getattr(logging, settings.LOG_LEVEL),
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

logger = logging.getLogger(__name__)

# Create FastAPI app
app = FastAPI(
    title="DuoClean Energy API",
    description="IoT Sensor Monitoring Platform API",
    version="1.0.0",
    docs_url="/docs",
    redoc_url="/redoc"
)

# Configure CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS_LIST,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include routers
app.include_router(auth.router)
app.include_router(users.router)
app.include_router(devices.router)
app.include_router(readings.router)


@app.get("/", tags=["Root"])
def root():
    """Root endpoint"""
    return {
        "message": "DuoClean Energy API",
        "version": "1.0.0",
        "docs": "/docs"
    }


@app.get("/health", response_model=schemas.HealthResponse, tags=["Health"])
def health_check():
    """Health check endpoint"""
    return {
        "status": "healthy",
        "timestamp": datetime.utcnow()
    }


@app.on_event("startup")
async def startup_event():
    """Run on application startup"""
    logger.info("=" * 60)
    logger.info("DuoClean Energy API starting up")
    logger.info(f"Environment: {settings.LOG_LEVEL}")
    logger.info(f"Database: {settings.POSTGRES_HOST}:{settings.POSTGRES_PORT}/{settings.POSTGRES_DB}")
    logger.info(f"CORS origins: {settings.CORS_ORIGINS_LIST}")
    logger.info("=" * 60)


@app.on_event("shutdown")
async def shutdown_event():
    """Run on application shutdown"""
    logger.info("DuoClean Energy API shutting down")
