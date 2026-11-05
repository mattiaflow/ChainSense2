«Prototipo sviluppato durante il tirocinio formativo presso Cinetix Group, con finalità di apprendimento e dimostrazione delle competenze tecniche acquisite.»

# ChainSense2

ChainSense2 è un progetto sviluppato durante un tirocinio per acquisire, validare, memorizzare e visualizzare dati provenienti da un dispositivo **Chain2Gate**.

Il sistema utilizza un **ESP32** come interfaccia hardware: il firmware comunica con Chain2Gate tramite UART, gestisce lo scambio delle Compact Frame **CF3** e **CF21**, mostra lo stato su un display OLED e inoltra i dati al computer in formato JSON. Sul computer, una serie di script Python salva le misure in un database SQLite e le rende consultabili tramite una dashboard Streamlit.

## Architettura del sistema

```text
Chain2Gate
    │
    │ UART 115200 baud, 8N1
    ▼
ESP32 ───► display OLED
    │
    │ USB seriale / JSON
    ▼
serial_reader.py
    │
    ▼
storage.py ───► python/data/chainsense.db
                         │
                         ▼
                  dashboard.py
                         │
                         ▼
                 dashboard Streamlit
```

## Funzionalità principali

- comunicazione seriale tra ESP32 e Chain2Gate;
- macchina a stati per la gestione ordinata del protocollo;
- invio della Compact Frame CF3;
- ricezione e validazione della Compact Frame CF21;
- gestione delle risposte ACK e NACK;
- visualizzazione dello stato operativo su display OLED;
- invio dei dati acquisiti al computer in formato JSON;
- lettura continua della porta seriale tramite Python;
- persistenza delle acquisizioni in un database SQLite;
- dashboard web realizzata con Streamlit e pandas;
- esecuzione opzionale della sola dashboard tramite Docker Compose.

## Componenti hardware

- scheda di sviluppo ESP32;
- dispositivo Chain2Gate;
- display OLED I²C;
- collegamento USB tra ESP32 e computer;
- collegamenti UART e massa comune tra ESP32 e Chain2Gate.

### Collegamenti

| Funzione | ESP32 | Dispositivo collegato |
| --- | --- | --- |
| UART RX | GPIO 16 | TX Chain2Gate |
| UART TX | GPIO 17 | RX Chain2Gate |
| I²C SDA | GPIO 21 | SDA OLED |
| I²C SCL | GPIO 22 | SCL OLED |
| Riferimento elettrico | GND | GND Chain2Gate e OLED |

> Prima dell'accensione verificare i livelli logici e il pinout della specifica versione hardware di Chain2Gate. TX e RX devono essere incrociati e i dispositivi devono condividere la massa.

## Comunicazione seriale e protocollo

La comunicazione ESP32–Chain2Gate usa i seguenti parametri:

- velocità: **115200 baud**;
- formato: **8N1** (8 bit dati, nessuna parità, 1 bit di stop);
- controllo di flusso: non previsto.

Il firmware è organizzato come una macchina a stati. In sintesi:

1. inizializza UART e display OLED;
2. prepara e invia una Compact Frame CF3;
3. attende la risposta del dispositivo;
4. interpreta ACK o NACK;
5. riceve la Compact Frame CF21;
6. controlla struttura, lunghezza e validità della frame;
7. estrae i dati utili e li pubblica sulla seriale USB in formato JSON;
8. aggiorna l'OLED e torna nello stato di attesa o ripete l'operazione in caso di errore.

La separazione in stati evita attese bloccanti e rende più chiara la gestione di timeout, frame incomplete e risposte non valide.

## Formato dei dati

Le acquisizioni vengono inviate dall'ESP32 al computer come oggetti JSON, uno per riga. Questo permette a `serial_reader.py` di distinguere i messaggi diagnostici dai record validi, decodificare i dati e passarli al livello di persistenza.

Il contenuto effettivo del record dipende dai campi estratti dalla CF21. In forma generale:

```json
{
  "tipo_frame": "CF21",
  "stato": "valida",
  "dati": {}
}
```

## Struttura del repository

```text
ChainSense2/
├── docs/
│   └── ChainSense2V3_manualeTecnico.pdf
├── firmware/
│   └── src/
│       └── main.cpp
├── python/
│   ├── dashboard.py
│   ├── serial_reader.py
│   ├── storage.py
│   ├── test_database.py
│   └── data/
│       └── chainsense.db          # creato durante l'utilizzo
├── Dockerfile
├── compose.yaml
├── requirements.txt
└── README.md
```

Il manuale tecnico completo è disponibile in [docs/ChainSense2V3_manualeTecnico.pdf](docs/ChainSense2V3_manualeTecnico.pdf).

## Installazione del firmware ESP32

Il firmware si trova in `firmware/src/main.cpp` ed è sviluppato per l'ambiente PlatformIO.

### Prerequisiti

- Visual Studio Code;
- estensione PlatformIO IDE, oppure PlatformIO Core da riga di comando;
- driver USB della scheda ESP32, se richiesto dal sistema operativo.

### Compilazione e caricamento

1. Aprire il progetto firmware con PlatformIO.
2. Collegare l'ESP32 al computer tramite USB.
3. Selezionare la scheda e la porta seriale corrette.
4. Compilare il firmware.
5. Caricarlo sull'ESP32.
6. Aprire il monitor seriale alla velocità prevista dalla configurazione del firmware.

Con PlatformIO Core, dalla directory del progetto firmware:

```bash
pio run
pio run --target upload
pio device monitor
```

## Installazione dell'ambiente Python

È consigliato Python 3 con un ambiente virtuale.

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
pip install -r requirements.txt
```

Il file `requirements.txt`, nella root del repository, contiene:

```text
streamlit
pandas
pyserial
```

## Acquisizione dei dati

Collegare l'ESP32 al computer, verificare la porta seriale configurata nello script e avviare:

```bash
python python/serial_reader.py
```

`serial_reader.py` riceve le righe JSON prodotte dal firmware e utilizza `storage.py` per inizializzare e aggiornare il database.

Il database viene creato nel percorso:

```text
python/data/chainsense.db
```

Sia `storage.py` sia `dashboard.py` costruiscono questo percorso a partire dalla directory del rispettivo file:

```python
Path(__file__).resolve().parent / "data" / "chainsense.db"
```

In questo modo l'acquisizione e la dashboard fanno riferimento allo stesso database indipendentemente dalla directory dalla quale vengono avviate.

Per verificare separatamente la creazione e l'accesso al database:

```bash
python python/test_database.py
```

## Avvio della dashboard

Dopo aver installato le dipendenze e creato il database:

```bash
streamlit run python/dashboard.py
```

Streamlit mostra nel terminale l'indirizzo locale della dashboard, normalmente:

```text
http://localhost:8501
```

La dashboard legge `python/data/chainsense.db`, elabora i dati con pandas e presenta le acquisizioni in forma consultabile.

## Dashboard con Docker

Il repository include un `Dockerfile` e un file `compose.yaml` per eseguire la dashboard in un container.

Il `Dockerfile` copia `python/dashboard.py` in:

```text
/app/dashboard.py
```

La dashboard cerca quindi il database in `/app/data/chainsense.db`. Docker Compose collega a tale directory il database presente sul computer:

```yaml
volumes:
  - ./python/data:/app/data:ro
```

Il suffisso `:ro` rende il database accessibile al container in sola lettura. La porta è pubblicata soltanto sull'interfaccia locale:

```yaml
ports:
  - "127.0.0.1:8501:8501"
```

Quando Docker è disponibile, la dashboard può essere costruita e avviata dalla root del repository con:

```bash
docker compose up --build
```

Quindi può essere aperta all'indirizzo [http://127.0.0.1:8501](http://127.0.0.1:8501).

> La cartella `python/data/` e il database `chainsense.db` devono esistere prima dell'avvio del container. Docker non è installato sul laptop usato per lo sviluppo: la configurazione è stata predisposta, ma non è ancora stata verificata in esecuzione su tale macchina.

## Stato dello sviluppo

Il progetto comprende:

- firmware ESP32 per la comunicazione con Chain2Gate;
- gestione CF3, CF21, ACK/NACK e validazione delle frame;
- uscita JSON sulla seriale USB;
- acquisizione e persistenza tramite Python e SQLite;
- dashboard Streamlit;
- configurazione Docker della dashboard;
- manuale tecnico V3.

Il lavoro è in fase di sviluppo e validazione. In particolare, l'esecuzione tramite Docker resta da provare su un sistema dotato di Docker.

## Documentazione

Per la descrizione dettagliata dell'hardware, del protocollo, della macchina a stati, del formato delle frame e delle prove svolte, consultare il [manuale tecnico V3](docs/ChainSense2V3_manualeTecnico.pdf).

## Autore

**Mattia**  
Progetto realizzato nell'ambito di un tirocinio.

## Licenza

Il codice sorgente di ChainSense2 è distribuito sotto licenza MIT. Per maggiori informazioni, consultare il file [LICENSE](LICENSE).

Il progetto è stato sviluppato da Mattia Floriani durante il tirocinio formativo presso Cinetix Group nel 2026.
