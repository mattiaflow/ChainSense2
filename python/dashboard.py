# Questa funzione serve per visualizzare i dati di potenza istantanea e lo storico delle misurazioni in un'interfaccia web.

from pathlib import (
    Path,
)  # serve per gestire i percorsi dei file in modo indipendente dal sistema operativo
from datetime import (
    datetime,
    timedelta,
)  # serve per gestire le date e gli orari, in particolare per calcolare il tempo trascorso dall'ultima misurazione

import pandas as pd  # serve per leggere il file csv
import streamlit as st  # serve per creare l'interfaccia web e da server
import sqlite3

# as st serve per abbreviare il nome del modulo streamlit, in modo da poterlo richiamare più facilmente nel codice

# Configurazione della pagina web

st.set_page_config(page_title="ChainSense2", page_icon="⚡", layout="wide")

st.markdown(
    """
<style>
    .main {
        background: linear-gradient(180deg, #0f172a 0%, #111827 100%);
        color: #e5e7eb;
    }
    .block-container {
        padding-top: 2rem;
        padding-bottom: 2rem;
    }
    h1 {
        color: #f8fafc;
        text-align: center;
        font-weight: 700;
        letter-spacing: 0.04em;
    }
    h3 {
        color: #cbd5e1;
        text-align: center;
    }
    div[data-testid="stMetric"] {
        background: rgba(148, 163, 184, 0.18);
        border: 1px solid rgba(148, 163, 184, 0.35);
        padding: 1rem;
        border-radius: 24px;
        box-shadow: 0 8px 24px rgba(0,0,0,0.08);
    }
    [data-testid="stHorizontalBlock"] > div {
        gap: 1rem;
    }
    .stProgress > div > div {
        background: linear-gradient(90deg, #60a5fa 0%, #34d399 35%, #facc15 70%, #f97316 100%);
    } 
    .stDataFrame {
        background: rgba(19, 23, 42, 0.8);
        border-radius: 12px;
        overflow: hidden;
    }
    .stButton > button {
        background: linear-gradient(90deg, #2563eb, #3b82f6);
        color: white;
        border: none;
        border-radius: 10px;
        font-weight: 600;
    }
    .stSelectbox > div {
        background: rgba(15, 23, 42, 0.8);
        border-radius: 10px;
    }
</style>
""",
    unsafe_allow_html=True,
)

# Percorso del file delle misurazioni
PERCORSO_DB = Path(__file__).resolve().parent / "data" / "chainsense.db"
# Intestazione della pagina
st.header("Tirocinio Mattia Floriani")
st.markdown("<h1>ChainSense2</h1>", unsafe_allow_html=True)
st.markdown(
    "<h3 style='text-align: center;'>Applicazione per la visualizzazione dei dati di potenza istantanea e storico delle misurazioni</h3>",
    unsafe_allow_html=True,
)

st.divider()  # serve per creare una linea di separazione tra le sezioni della pagina web


def carica_misurazioni():
    """Legge le misurazioni dal database SQLite."""

    if not PERCORSO_DB.exists():
        return pd.DataFrame()

    connessione = sqlite3.connect(PERCORSO_DB)

    try:
        dati = pd.read_sql_query(
            """
            SELECT timestamp AS Timestamp,
                   cf AS CF,
                   potenza_w AS Potenza_W
            FROM misurazioni
            ORDER BY timestamp ASC
            """,
            connessione,
        )

    finally:
        connessione.close()

    if dati.empty:
        return dati

    dati["Timestamp"] = pd.to_datetime(dati["Timestamp"], errors="coerce")

    dati = dati.dropna(subset=["Timestamp"])

    return dati


def mostra_potenza(dati):

    # Estrazione dell'ultima misurazione
    ultima_misura = dati.iloc[-1]

    potenza = ultima_misura["Potenza_W"]
    tipo_cf = ultima_misura["CF"]
    timestamp = ultima_misura["Timestamp"]
    # Potenza massima
    potenza_massima = dati["Potenza_W"].max()

    # Calcolo età dell'ultima misurazione
    ultima_ricezione = timestamp.to_pydatetime()
    ora_attuale = datetime.now()

    secondi_trascorsi = (ora_attuale - ultima_ricezione).total_seconds()

    # Stato della misurazione
    if secondi_trascorsi < 30:
        st.success("Misurazione recente", width=300)

    elif secondi_trascorsi < 120:
        st.warning("Misurazione non recente", width=300)

    else:
        st.error(f"Misurazione obsoleta: da {secondi_trascorsi:.0f} secondi", width=300)

    col1, col2 = st.columns(2)

    with col1:
        st.metric(label="Potenza istantanea", value=f"{potenza} W")

    with col2:
        st.metric(label="Potenza massima", value=f"{potenza_massima} W")

    # Indicatore grafico della potenza
    potenza_riferimento = 6000

    percentuale = potenza / potenza_riferimento * 100
    progresso = min(max(percentuale / 100, 0.0), 1.0)

    st.progress(progresso, text=f"Utilizzo potenza di riferimento: {percentuale:.1f}%")

    # Informazioni dell'ultima ricezione
    st.write(f"Compact Frame: CF{tipo_cf}")
    st.write(f"Ultima misurazione: {timestamp}")


def filtra_misurazioni(dati, intervallo):
    """Restituisce i dati degli ultimi minuti selezionati."""
    inizio_intervallo = datetime.now() - timedelta(minutes=intervallo)
    return dati[dati["Timestamp"] >= inizio_intervallo]


def mostra_grafico(dati):
    """Disegna la potenza nel tempo usando le misurazioni filtrate."""
    # Creazione del grafico a linee con Streamlit
    st.subheader("Andamento della potenza")
    st.line_chart(
        dati,
        x="Timestamp",
        y="Potenza_W",
        x_label="Orario",
        y_label="Potenza (W)",
        height=400,
        # width=900,
        alt="Andamento temporale della potenza elettrica istantanea",
    )


def mostra_statistiche(dati):
    """Calcola e mostra la potenza minima, media e massima dei campioni."""
    # statistiche delle misurazioni selezionate
    potenza_minima = dati["Potenza_W"].min()
    potenza_media = dati["Potenza_W"].mean()
    potenza_massima = dati["Potenza_W"].max()

    # creazione del DataFrame per il grafico a barre
    statistiche = pd.DataFrame(
        {
            "Tipo": ["Minima", "Media", "Massima"],
            "Potenza (W)": [potenza_minima, potenza_media, potenza_massima],
        }
    )
    st.subheader("Statistiche della potenza")
    st.bar_chart(
        statistiche,
        x="Tipo",
        y="Potenza (W)",
        x_label="Statistica",
        y_label="Potenza (W)",
        height=400,
        # width=600,
        alt="Statistiche della potenza elettrica istantanea",
    )
    # st.write(
    #    f"Potenza minima: {potenza_minima:.2f} W, Potenza media: {potenza_media:.2f} W, Potenza massima: {potenza_massima:.2f} W"
    # )


# Mostra una tabella con gli ultimi 10 records
def mostra_tabella(dati):
    """Mostra le ultime 10 misurazioni dello storico in ordine decrescente."""
    # tabella delle ultime misurazioni
    st.subheader("Ultime 10 misurazioni")

    ultime_10 = dati.tail(10)

    # ordinamento dalla più recente alla più vecchia
    ultime_10 = ultime_10.sort_values("Timestamp", ascending=False)
    st.dataframe(
        ultime_10,
        hide_index=True,
        width="stretch",
        alt="Ultime 10 misurazioni, dalla pi\u00f9 recente alla pi\u00f9 vecchia",
    )


def mostra_footer():
    """Visualizza il footer informativo della dashboard."""

    st.divider()

    col1, col2 = st.columns(2)

    with col1:
        st.markdown("**ChainSense2 v1.0**")
        st.caption("Energy Monitoring Dashboard")

    with col2:
        st.markdown(
            "<div style='text-align: right;'>"
            "<strong>Mattia Floriani</strong>"
            "</div>",
            unsafe_allow_html=True,
        )

        st.markdown(
            "<div style='text-align: right; font-size: 13px;'>"
            "Progetto di tirocinio · 2026"
            "</div>",
            unsafe_allow_html=True,
        )

    st.caption(
        "InternShip ·ESP32 · Python · Streamlit · Cinetix Group · Istituto Marie Curie Pergine Valsugana",
        text_alignment="center",
    )


def esporta_csv(dati):
    """Permette di scaricare le misurazioni in formato CSV."""

    csv = dati.tail(10).to_csv(index=False, sep=";").encode("utf-8-sig")

    st.download_button(
        label="Esporta misurazioni CSV",
        data=csv,
        file_name="ChainSense2_misurazioni.csv",
        mime="text/csv",
        icon=":material/download:",
    )


# Aggiorna soltanto il monitoraggio ogni 3 secondi.
@st.fragment(run_every="3s")
def aggiorna_monitoraggio():
    """Coordina il caricamento, il filtro e la visualizzazione dei dati."""
    dati = carica_misurazioni()

    if dati.empty:
        st.warning("Nessuna misurazione disponibile.")
        return

    mostra_potenza(dati)

    st.divider()

    intervallo = st.selectbox(
        "Intervallo temporale",
        options=[5, 15, 30, 60],
        index=1,
        format_func=lambda minuti: f"ultimi {minuti} minuti",
    )

    misure_filtrate = filtra_misurazioni(dati, intervallo)

    if not misure_filtrate.empty:
        grafico_col, stats_col = st.columns([2, 1])
        with grafico_col:
            mostra_grafico(misure_filtrate)
        with stats_col:
            mostra_statistiche(misure_filtrate)

        st.divider()
        mostra_tabella(misure_filtrate)
        
        st.divider()
        st.subheader("Esportazione dati")
        
        esporta_csv(misure_filtrate)

    else:
        st.info("Nessuna misurazione disponibile nell'intervallo selezionato.")

    
    mostra_footer()


# Main
aggiorna_monitoraggio()
