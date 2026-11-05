
# Immagine di base con Python
FROM python:3.12-slim

# Directory di lavoro nel container
WORKDIR /app

# Copia il file delle dipendenze
COPY requirements.txt .

# Installa le librerie Python
RUN pip install --no-cache-dir -r requirements.txt

# Copia l'applicazione Streamlit
COPY python/dashboard.py .

# Documenta la porta utilizzata
EXPOSE 8501

# Avvia automaticamente la dashboard
CMD ["streamlit", "run", "dashboard.py", "--server.address=0.0.0.0"]
