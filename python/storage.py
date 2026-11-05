# questa funzione serve per salvare le misurazioni in un file DataBase sqlite

import sqlite3
from datetime import datetime
from pathlib import Path

PERCORSO_DB = Path(__file__).resolve().parent / "data" / "chainsense.db"


def inizializza_database():
    # Crea la cartella data, se non esiste
    PERCORSO_DB.parent.mkdir(parents=True, exist_ok=True)

    # Apre o crea il database
    with sqlite3.connect(PERCORSO_DB) as connessione:
        connessione.execute("""
            CREATE TABLE IF NOT EXISTS misurazioni (
                id INTEGER PRIMARY KEY,
                timestamp TEXT,
                cf INTEGER,
                potenza_w INTEGER
            )
        """)
        connessione.commit()



def salva_misurazione(
    # inserisce i dati nel record e lo salva nel DB
    timestamp: datetime,
    tipo_cf: int,
    potenza: int
) -> None:

    connessione = sqlite3.connect(PERCORSO_DB)

    try:
        connessione.execute("""
            INSERT INTO misurazioni (timestamp, cf, potenza_w)
            VALUES (?, ?, ?)
        """, (
            timestamp.isoformat(timespec="seconds"),
            tipo_cf,
            potenza
        ))

        connessione.commit()

    finally:
        connessione.close()
        

def leggi_misurazioni():
    connessione = sqlite3.connect(PERCORSO_DB)

    try:
        risultato = connessione.execute(
            "SELECT * FROM misurazioni"
        )

        righe = risultato.fetchall()

        return righe

    finally:
        connessione.close()


