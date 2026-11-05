# ChainSense2

Sistema di monitoraggio della potenza elettrica basato su **ESP32** e
**Chain2Gate Engine**. Il progetto riceve le Compact Frame provenienti da un
contatore Open Meter, ne verifica l'integrità, estrae la potenza istantanea e
mostra le misurazioni su un display OLED e su una dashboard web.

Il progetto è stato sviluppato da **Mattia Floriani** durante il tirocinio
formativo presso **Cinetix Group**, nell'anno scolastico 2026/2027.

## Funzionalità

- ricezione delle Compact Frame tramite protocollo CCP su UART;
- verifica del checksum e invio di risposte ACK/NACK;
- riconoscimento delle frame CF3 e CF21;
- visualizzazione della potenza su display OLED SH1106;
- segnalazione dello stato tramite LED;
- invio delle misurazioni dall'ESP32 al computer in formato JSON;
- salvataggio dello storico in un database SQLite;
- dashboard Streamlit con grafici, statistiche ed esportazione CSV.

## Architettura

```text
Contatore Open Meter
        |
        v
Chain2Gate Engine
        | UART / CCP
        v
      ESP32 --------> Display OLED e LED di stato
        |
        | USB seriale / JSON
        v
Acquisizione Python
        |
        v
 Database SQLite
        |
        v
Dashboard Streamlit
```

Esempio di dato inviato dal firmware:

```json
{"cf": 21, "potenza_w": 1250}
```

## Componenti principali

- ESP32 DOIT DevKit V1;
- modulo Chain2Gate Engine GRID Edition;
- display OLED SH1106 128x64 con interfaccia I2C;
- un LED giallo e un LED rosso;
- computer con Python 3 e collegamento USB all'ESP32.

## Collegamenti ESP32

| Funzione | Pin ESP32 |
| --- | ---: |
| UART2 RX | GPIO 16 |
| UART2 TX | GPIO 17 |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| LED rosso | GPIO 12 |
| LED giallo | GPIO 14 |

La UART del firmware è configurata a **115200 baud, 8N1**.

## Struttura del progetto

```text
ChainSense2/
|-- firmware/
|   |-- platformio.ini
|   `-- src/
|       `-- main .cpp
|-- python/
|   |-- dashboard.py
|   |-- serial_reader.py
|   |-- storage.py
|   `-- test_database.py
|-- Dockerfile/
|   |-- Dockerfile
|   |-- compose.yaml
|   `-- requirements.txt
|-- .gitignore
`-- README.md
```

## Installazione

### 1. Firmware ESP32

Il firmware usa [PlatformIO](https://platformio.org/) e la libreria U8g2,
già dichiarata in `firmware/platformio.ini`.

Da Visual Studio Code:

1. installare l'estensione PlatformIO IDE;
2. aprire la cartella `firmware` come progetto PlatformIO;
3. collegare l'ESP32 tramite USB;
4. compilare e caricare il firmware sulla scheda.

In alternativa, con PlatformIO Core:

```bash
cd firmware
pio run
pio run --target upload
```

> **Nota:** PlatformIO usa normalmente il nome `src/main.cpp`. Se il file non
> viene rilevato durante la compilazione, rinominare `main .cpp` in `main.cpp`.

### 2. Ambiente Python

Creare un ambiente virtuale dalla root del progetto:

```bash
python -m venv .venv
```

Attivarlo su Windows:

```powershell
.venv\Scripts\Activate.ps1
```

Oppure su Linux/macOS:

```bash
source .venv/bin/activate
```

Installare le dipendenze:

```bash
python -m pip install -r Dockerfile/requirements.txt pyserial
```

## Utilizzo

### 1. Configurare la porta seriale

Aprire `python/serial_reader.py` e impostare `PORTA` con la porta assegnata
all'ESP32. Per esempio:

```python
PORTA = "COM3"       # Windows
# PORTA = "/dev/ttyUSB0"  # Linux
```

La velocità deve rimanere impostata a `115200` baud, come nel firmware.

### 2. Avviare l'acquisizione

Dalla root del progetto eseguire:

```bash
python python/serial_reader.py
```

Il programma crea automaticamente il database
`python/data/chainsense.db` e salva ogni misurazione ricevuta.

### 3. Avviare la dashboard

In un secondo terminale, con lo stesso ambiente virtuale attivo, eseguire:

```bash
streamlit run python/dashboard.py
```

La dashboard è disponibile normalmente all'indirizzo
<http://localhost:8501> e si aggiorna ogni 3 secondi.

## Database

Le misurazioni sono salvate nella tabella `misurazioni` con i campi seguenti:

| Campo | Descrizione |
| --- | --- |
| `id` | identificativo progressivo |
| `timestamp` | data e ora della ricezione |
| `cf` | tipo di Compact Frame ricevuta |
| `potenza_w` | potenza istantanea in watt |

Per visualizzare rapidamente gli ultimi dieci record:

```bash
python python/test_database.py
```

## Docker

La cartella `Dockerfile` contiene una prima configurazione per eseguire la
dashboard in un container. Prima dell'uso è necessario allineare il contesto
di build con i file presenti in `python/`, poiché il Dockerfile copia
`dashboard.py` dal proprio contesto mentre il file si trova attualmente in
un'altra cartella.

## Stato del progetto

ChainSense2 è in fase di sviluppo. Le funzionalità principali di acquisizione,
salvataggio e visualizzazione sono già presenti; configurazione Docker, test
automatici e gestione della porta seriale tramite configurazione esterna sono
ancora da completare.

## Autore

**Mattia Floriani**  
Istituto Marie Curie, Pergine Valsugana (TN)  
Tirocinio formativo presso Cinetix Group, Borgo Valsugana (TN)

## Licenza

Non è ancora stata definita una licenza per il progetto.
