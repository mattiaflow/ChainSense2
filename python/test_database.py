#codice risevato al test lettura DB
import sqlite3
from pathlib import Path

percorso = Path(__file__).resolve().parent / "data" / "chainsense.db"

connessione = sqlite3.connect(percorso)

risultato = connessione.execute("""
    SELECT *
    FROM misurazioni
    ORDER BY id DESC
    LIMIT 10
""")

for riga in risultato:
    print(riga)

connessione.close()
