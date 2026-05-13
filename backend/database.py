import sqlite3
from datetime import datetime, timedelta
from typing import List, Optional


DATABASE_NAME = "climate.db"


def get_connection():
    connection = sqlite3.connect(DATABASE_NAME)
    connection.row_factory = sqlite3.Row
    return connection


def create_tables():
    connection = get_connection()
    cursor = connection.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            temperature REAL NOT NULL,
            humidity REAL NOT NULL,
            timestamp TEXT NOT NULL
        )
    """)

    connection.commit()
    connection.close()


def insert_reading(temperature: float, humidity: float) -> dict:
    timestamp = datetime.now().isoformat()

    connection = get_connection()
    cursor = connection.cursor()

    cursor.execute(
        """
        INSERT INTO readings (temperature, humidity, timestamp)
        VALUES (?, ?, ?)
        """,
        (temperature, humidity, timestamp)
    )

    connection.commit()

    reading_id = cursor.lastrowid

    connection.close()

    return {
        "id": reading_id,
        "temperature": temperature,
        "humidity": humidity,
        "timestamp": timestamp
    }


def get_latest_reading() -> Optional[dict]:
    connection = get_connection()
    cursor = connection.cursor()

    cursor.execute("""
        SELECT id, temperature, humidity, timestamp
        FROM readings
        ORDER BY timestamp DESC
        LIMIT 1
    """)

    row = cursor.fetchone()
    connection.close()

    if row is None:
        return None

    return dict(row)


def get_readings_last_24h() -> List[dict]:
    cutoff_time = datetime.now() - timedelta(hours=24)
    cutoff_time_text = cutoff_time.isoformat()

    connection = get_connection()
    cursor = connection.cursor()

    cursor.execute(
        """
        SELECT id, temperature, humidity, timestamp
        FROM readings
        WHERE timestamp >= ?
        ORDER BY timestamp ASC
        """,
        (cutoff_time_text,)
    )

    rows = cursor.fetchall()
    connection.close()

    return [dict(row) for row in rows]