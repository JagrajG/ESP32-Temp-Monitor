from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

from database import (
    create_tables,
    insert_reading,
    get_latest_reading,
    get_readings_last_24h,
)


app = FastAPI()


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


class ReadingCreate(BaseModel):
    temperature: float
    humidity: float


@app.on_event("startup")
def startup():
    create_tables()


@app.get("/")
def root():
    return {"message": "ESP32 Climate Monitor API is running with SQLite"}


@app.post("/readings")
def create_reading(reading: ReadingCreate):
    saved_reading = insert_reading(
        temperature=reading.temperature,
        humidity=reading.humidity
    )

    return {
        "message": "Reading saved",
        "reading": saved_reading
    }


@app.get("/readings/latest")
def latest_reading():
    reading = get_latest_reading()

    if reading is None:
        raise HTTPException(status_code=404, detail="No readings available")

    return reading


@app.get("/readings/last-24h")
def last_24h_readings():
    return get_readings_last_24h()