from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from sqlalchemy import create_engine, Column, Integer, Float, Boolean, DateTime
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
from datetime import datetime
from dateutil.parser import parse  # Para analizar fechas

# Configurar conexión a PostgreSQL
DATABASE_URL = "postgresql://postgres:mafe1219@localhost:5432/vibraciones_db"

engine = create_engine(DATABASE_URL)
SessionLocal = sessionmaker(bind=engine)
Base = declarative_base()

# Modelo de datos actualizado
class Reading(Base):
    __tablename__ = "readings"
    id = Column(Integer, primary_key=True, index=True)
    magnitud = Column(Float)
    alerta = Column(Boolean)
    tiempo = Column(DateTime, default=datetime.utcnow)

Base.metadata.create_all(bind=engine)

# FastAPI
app = FastAPI()

# CORS para permitir acceso desde tu HTML
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Mejor limitar a tus dominios frontend
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Esquema de entrada reducido
class ReadingIn(BaseModel):
    magnitud: float
    alerta: bool

@app.post("/api/readings")
def create_reading(reading: ReadingIn):
    db = SessionLocal()
    try:
        new_reading = Reading(**reading.dict())
        db.add(new_reading)
        db.commit()
        db.refresh(new_reading)
        return {"message": "Datos recibidos", "id": new_reading.id}
    finally:
        db.close()

@app.get("/api/readings")
def get_readings(
    start_date: str = None,
    end_date: str = None,
    min_magnitud: float = 0.0,
    max_magnitud: float = 10.0
):
    db = SessionLocal()
    try:
        query = db.query(Reading).filter(
            Reading.magnitud >= min_magnitud,
            Reading.magnitud <= max_magnitud
        )

        if start_date:
            try:
                start_dt = parse(start_date)
                query = query.filter(Reading.tiempo >= start_dt)
            except Exception as e:
                return {"error": f"start_date inválida: {e}"}

        if end_date:
            try:
                end_dt = parse(end_date)
                query = query.filter(Reading.tiempo <= end_dt)
            except Exception as e:
                return {"error": f"end_date inválida: {e}"}

        results = query.order_by(Reading.tiempo.desc()).limit(500).all()

        # Aquí la corrección clave: convertir fecha a ISO string
        return [
            {
                "tiempo": r.tiempo.isoformat(),
                "magnitud": r.magnitud,
                "alerta": r.alerta
            }
            for r in results
        ]
    finally:
        db.close()

@app.get("/api/test")
def test():
    return {"message": "Servidor funcionando"}
