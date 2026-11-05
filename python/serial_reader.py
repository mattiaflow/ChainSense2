#questa funzione serve per leggere i dati inviati dall'ESP32 tramite la porta seriale, decodificarli e salvarli dataBase.
import json
import serial

from datetime import datetime
from storage import salva_misurazione, inizializza_database 


# Configurazione seriale
PORTA = "COM3"
VELOCITA = 115200

# Funzione principale
def main() -> None:
    
    #Inizializza il data base
    inizializza_database()

    try:
        # Apertura della porta seriale
        with serial.Serial(PORTA, VELOCITA, timeout=1) as esp32:

            print("Connessione ESP32 avviata...")

            while True:

                # readline attende i dati fino al timeout, evitando un ciclo a vuoto.

                messaggio = esp32.readline().decode(
                    "utf-8", errors="replace"
                ).strip()

                # Ignora i messaggi di debug
                if not messaggio.startswith("{"):
                    continue

                try:
                    dati = json.loads(messaggio)
                    # Estrazione dei valori
                    tipo_cf = int(dati["cf"])
                    potenza = int(dati["potenza_w"])
                    timestamp = datetime.now()

                    # Persistenza dei dati
                    salva_misurazione(timestamp, tipo_cf, potenza)
                    # Stampa dei dati ricevuti
                    print(f"CF ricevuta: {tipo_cf}")
                    print(f"Potenza: {potenza} W")
                    print(f"Timestamp: {timestamp}")
                    print("Misurazione salvata.")
                    print("-----------------------------")
                
                except json.JSONDecodeError as errore:
                    print(f"Errore decodifica JSON: {errore}")

                except (KeyError, TypeError, ValueError) as errore:
                    print(f"Errore nei dati ricevuti: {errore}")

    except serial.SerialException as errore:
        print(f"Errore comunicazione seriale: {errore}")

    except OSError as errore:
        print(f"Errore accesso al file: {errore}")

    except KeyboardInterrupt:
        print("\nAcquisizione interrotta dall'utente.")

#Richiama la funzione principale se il file viene eseguito direttamente
if __name__ == "__main__":
    main()


