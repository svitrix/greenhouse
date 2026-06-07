from contextlib import asynccontextmanager
from collections.abc import AsyncIterator

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.db import init_engine
from app.routes import api_router


@asynccontextmanager
async def lifespan(_app: FastAPI) -> AsyncIterator[None]:
    init_engine()
    yield


app = FastAPI(
    title="Greenhouse Telemetry Backend",
    version="0.1.0",
    lifespan=lifespan,
)

# CORS: Vite dev (:5173) and the dockerised nginx frontend (:3000).
# Production origins will be moved to Settings in D-cloud.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://localhost:5173"],
    allow_credentials=False,
    allow_methods=["GET", "POST", "PATCH", "DELETE", "OPTIONS"],
    allow_headers=["Authorization", "Content-Type"],
)

app.include_router(api_router)


@app.get("/")
async def root() -> dict[str, str]:
    return {"service": "greenhouse-backend", "version": "0.1.0"}
