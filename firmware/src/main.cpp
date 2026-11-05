
/**
 * @file main.cpp
 * @brief Sistema di monitoraggio energetico basato su ESP32 e Chain2Gate.
 *
 * @details
 * Applicazione embedded sviluppata per la ricezione, decodifica e
 * visualizzazione dei dati provenienti da un contatore elettrico
 * Open Meter tramite il modulo Chain2Gate Engine.
 *
 * Il sistema gestisce:
 * - Comunicazione seriale UART mediante protocollo CCP.
 * - Ricezione delle Compact Frame (CF).
 * - Verifica dell'integrita' dei messaggi tramite checksum.
 * - Gestione delle risposte ACK/NACK.
 * - Decodifica e interpretazione dei dati energetici.
 * - Visualizzazione delle informazioni su display OLED.
 * - Segnalazione dello stato operativo tramite LED.
 * - Gestione non bloccante mediante macchina a stati.
 *
 * @author Mattia Floriani
 * @version 1.0
 * @date 05/10/2026
 *
 * @note Progetto sviluppato durante il tirocinio formativo
 *       presso Cinetix Group, Borgo Valsugana (TN).
 *
 * @note Istituto Marie Curie - Pergine Valsugana (TN).
 *       Classe 5^ ASR - Informatica e Telecomunicazioni.
 *       Anno scolastico 2026/2027.
 *
 * @see Chain2Gate Engine GRID Edition - MAC S.r.l. (2023).
 */

#include <Arduino.h> // Libreria principale per lo sviluppo su piattaforma Arduino/ESP32.
#include <U8g2lib.h> // Libreria per la gestione del display OLED SH1106, con supporto a buffer completo e interfaccia I2C hardware.
#include <Wire.h> // Libreria per la comunicazione I2C, utilizzata per il display OLED.

// Configurazione hardware, delimitatori e temporizzazioni (millisecondi).
constexpr int RXD2 = 16;
constexpr int TXD2 = 17;
constexpr int ledRosso = 12;
constexpr int ledGiallo = 14;
constexpr uint8_t carattereStart = 0xF7;
constexpr uint8_t carattereEnd = 0xFB;
constexpr unsigned long timeoutRicezione = 300;
constexpr unsigned long durataPresentazione = 2000;
constexpr unsigned long intervalloTestLed = 200;
constexpr unsigned long durataLogo = 2000;
constexpr unsigned long durata_RX = 200;
constexpr unsigned long durataLedErrore = 3000;

/// Risposta positiva a CCP_CF_NOTIFICATION, incluso il checksum.
constexpr uint8_t tramaAck[] =
    {
        0xF7, 0x01, 0x7F, 0x4C, 0xEF, 0xFB, 0xBB};

/// Risposta negativa a CCP_CF_NOTIFICATION, incluso il checksum.
constexpr uint8_t tramaNack[] =
    {
        0xF7, 0x01, 0x7F, 0x4C, 0xFF, 0xFB, 0xCB};

/// Stati principali del dispositivo.
enum class StatoSistema
{
    AVVIO,
    OPERATIVO,
    ERRORE
};
StatoSistema statoSistema = StatoSistema::AVVIO;

/// Fasi della presentazione e del test LED non bloccante.
enum class StatoAvvio
{
    PRESENTAZIONE,
    TEST_LED,
    LOGO,
    COMPLETATO
};
StatoAvvio statoAvvio = StatoAvvio::PRESENTAZIONE;

/// Fasi della ricezione di una trama delimitata da START ed END.
enum class StatoRicezione
{
    ATTESA_STX,
    RICEZIONE_DATI,
    ATTESA_CHECKSUM,
    TRAMA_COMPLETA
};
StatoRicezione statoRicezione = StatoRicezione::ATTESA_STX;

/// Display OLED SH1106 con buffer completo e interfaccia I2C hardware.
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// Buffer e stato della ricezione seriale.
uint8_t byteRicevuto;
uint8_t bufferRx[384];
size_t indiceRx = 0;
uint8_t checksumCalcolato = 0;
unsigned long timerUltimoByte = 0;
bool nuoviDatiSeriali = false;

// Stato e timer della sequenza di avvio.
unsigned long timerAvvio = 0;
unsigned long timerTestLed = 0;
unsigned long timerLogo = 0;

bool presentazioneIniziata = false;
bool stampalogo = false;
bool ledAccesi = false;

int numeroLampeggi = 0;

// Stato e timer delle segnalazioni LED.
unsigned long timerLedRX = 0;
unsigned long timerLedErrore = 0;
bool lampeggioRx = false;
bool segnalazioneErrore = false;

// Contatori delle ricezioni e degli errori.
uint32_t trameValide = 0;
uint32_t erroriChecksum = 0;
uint32_t erroriTimeout = 0;
uint32_t erroriOverflow = 0;
uint32_t eventiTotali = 0;
uint32_t trameIgnorate = 0;

// Dati estratti dal payload dell'ultima trama.
uint8_t tipoCf = 0;
uint16_t potenzaIstantanea = 0;

// Prototipi delle funzioni di gestione e visualizzazione.
void gestioneAvvio();
void gestioneSeriale();
void aggiornaSerialeMonitor();
void aggiornaDisplay();
void aggiornaLed();
void riceviDati();
void resetRicezione();
void parseFrame();
void inviaDatiPython();

/**
 * @brief Inizializza I2C, display, porte seriali e pin dei LED.
 */
void setup()
{
    Wire.begin(21, 22); // SDA: GPIO 21; SCL: GPIO 22.
    oled.begin();

    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

    pinMode(ledRosso, OUTPUT);
    pinMode(ledGiallo, OUTPUT);
    Serial.println("Inizializzazione completata....");
}

/**
 * @brief Esegue la macchina a stati e aggiorna le uscite dopo una trama completa.
 */
void loop()
{
    switch (statoSistema)
    {
    case StatoSistema::AVVIO:
        gestioneAvvio();
        break;
    case StatoSistema::OPERATIVO:
        gestioneSeriale();

        if (nuoviDatiSeriali)
        {
            parseFrame();
            aggiornaSerialeMonitor();
            inviaDatiPython();
            aggiornaDisplay();
            resetRicezione();
        }

        aggiornaLed();
        break;
    case StatoSistema::ERRORE:
        break;
    }
}

/**
 * @brief Avanza la presentazione, il test LED e il logo usando timer non bloccanti.
 */
void gestioneAvvio()
{
    switch (statoAvvio)
    {
    case StatoAvvio::PRESENTAZIONE:
    {
        if (!presentazioneIniziata)
        {
            oled.clearBuffer();

            oled.setFont(u8g2_font_6x10_tr);
            oled.drawStr(25, 10, "ChainSense2");

            oled.drawHLine(0, 14, 128);

            oled.drawStr(12, 30, "Avvio sistema...");
            oled.drawStr(12, 45, "Test LED in corso");

            oled.sendBuffer();

            timerAvvio = millis();
            presentazioneIniziata = true;
        }

        if (millis() - timerAvvio >= durataPresentazione)
        {
            statoAvvio = StatoAvvio::TEST_LED;
            timerTestLed = millis();
        }

        break;
    }
    case StatoAvvio::TEST_LED:

        if (millis() - timerTestLed >= intervalloTestLed)
        {
            timerTestLed = millis();

            ledAccesi = !ledAccesi;

            digitalWrite(ledGiallo, ledAccesi ? HIGH : LOW);
            digitalWrite(ledRosso, ledAccesi ? HIGH : LOW);

            if (!ledAccesi)
            {
                numeroLampeggi++;
            }

            if (numeroLampeggi >= 3)
            {
                statoAvvio = StatoAvvio::LOGO;
            }
        }
        break;
    case StatoAvvio::LOGO:
        if (!stampalogo)
        {
            oled.clearBuffer();

            oled.setFont(u8g2_font_ncenB10_tr);
            oled.drawStr(13, 19, "ChainSense2");

            oled.setFont(u8g2_font_6x10_tr);
            oled.drawStr(7, 37, "Energy Data Receiver");

            oled.drawHLine(9, 44, 110);
            oled.drawStr(4, 58, "internshiProject M.F.");

            oled.sendBuffer();
            timerLogo = millis();
            stampalogo = true;
        }

        if (millis() - timerLogo >= durataLogo)
        {
            statoAvvio = StatoAvvio::COMPLETATO;
        }
        break;

    case StatoAvvio::COMPLETATO:
    {
        oled.clearBuffer();

        oled.setFont(u8g2_font_6x10_tr);
        oled.drawStr(12, 25, "ChainSense2");
        oled.drawHLine(0, 31, 128);

        oled.drawStr(12, 47, "In attesa dati...");

        oled.sendBuffer();

        statoSistema = StatoSistema::OPERATIVO;
        break;
    }
    }
}

/**
 * @brief Invoca la ricezione dei byte disponibili sulla porta del contatore.
 */
void gestioneSeriale()
{
    riceviDati();
}

/**
 * @brief Stampa dati e statistiche quando una nuova trama è disponibile.
 */
void aggiornaSerialeMonitor()
{
    if (!nuoviDatiSeriali)
    {
        return;
    }
    Serial.println();
    Serial.println("-------- RICEZIONI CHAIN2 -------- ");
    Serial.print("Byte ricevuti: ");
    Serial.println(indiceRx);
    Serial.print("Codice CCP 0x");
    Serial.println(bufferRx[3], HEX);

    Serial.print("Tipo Compact Frame: ");
    switch (tipoCf)
    {
    case 21:
        Serial.println("CF21 - Variazione potenza");
        break;

    case 3:
        Serial.println("CF3 - Misure quartorarie");
        break;

    default:
        Serial.println("Non riconosciuta");
        break;
    }

    if (tipoCf == 21 || tipoCf == 3)
    {
        Serial.print("Potenza istantanea: ");
        Serial.print(potenzaIstantanea);
        Serial.print(" W - ");
        Serial.print(potenzaIstantanea / 1000.0, 3);
        Serial.println(" kW");
    }

    Serial.print("Checksum verificato: 0x");
    if (bufferRx[indiceRx - 1] < 0x10)
    {
        Serial.print('0');
    }
    Serial.println(bufferRx[indiceRx - 1], HEX);

    eventiTotali = trameValide + trameIgnorate + erroriChecksum +
                   erroriTimeout + erroriOverflow;

    Serial.println("--- STATISTICHE RX ---");

    Serial.print("Eventi totali: ");
    Serial.println(eventiTotali);

    Serial.print("Trame valide: ");
    Serial.println(trameValide);

    Serial.print("Trame ignorate: ");
    Serial.println(trameIgnorate);

    Serial.print("Errori checksum: ");
    Serial.println(erroriChecksum);

    Serial.print("Errori timeout: ");
    Serial.println(erroriTimeout);

    Serial.print("Errori overflow: ");
    Serial.println(erroriOverflow);

    Serial.println("-------Fine Trasmissione-------");
    Serial.println();
}

/**
 * @brief Riceve una trama Chain 2, verifica il checksum e gestisce ACK/NACK.
 *
 * @details Elabora i byte disponibili fino alla prima notifica CF valida.
 * Timeout e overflow ripristinano la ricezione; le altre trame vengono
 * ignorate. Il checksum somma i dati modulo 256, escludendo START ed END.
 */
void riceviDati()
{
    // Controlla anche quando non arrivano byte, inclusa l'attesa del checksum.
    // La sottrazione unsigned resta valida anche al rollover di millis().
    if ((statoRicezione == StatoRicezione::RICEZIONE_DATI ||
         statoRicezione == StatoRicezione::ATTESA_CHECKSUM) &&
        Serial2.available() == 0 &&
        millis() - timerUltimoByte >= timeoutRicezione)
    {
        erroriTimeout++;

        Serial.print("ERRORE TIMEOUT RX - Totale: ");
        Serial.println(erroriTimeout);

        segnalazioneErrore = true;
        timerLedErrore = millis();

        resetRicezione();
    }

    while (Serial2.available() > 0)
    {
        byteRicevuto = Serial2.read();

        // Dopo il reset, il byte corrente può avviare una nuova trama START.
        if ((statoRicezione == StatoRicezione::RICEZIONE_DATI ||
             statoRicezione == StatoRicezione::ATTESA_CHECKSUM) &&
            indiceRx >= sizeof(bufferRx))
        {
            erroriOverflow++;

            Serial.println("ERRORE: overflow buffer RX");

            segnalazioneErrore = true;
            timerLedErrore = millis();

            resetRicezione();
        }

        switch (statoRicezione)
        {
        case StatoRicezione::ATTESA_STX:
        {
            if (byteRicevuto == carattereStart)
            {
                indiceRx = 0;
                checksumCalcolato = 0;
                bufferRx[indiceRx++] = byteRicevuto;
                timerUltimoByte = millis();

                statoRicezione = StatoRicezione::RICEZIONE_DATI;
            }
            break;
        }
        case StatoRicezione::RICEZIONE_DATI:
        {
            bufferRx[indiceRx++] = byteRicevuto;
            timerUltimoByte = millis();

            if (byteRicevuto == carattereEnd)
            {
                statoRicezione = StatoRicezione::ATTESA_CHECKSUM;
            }
            else
            {
                // Somma modulo 256 dei soli dati: START ed END sono esclusi.
                checksumCalcolato =
                    static_cast<uint8_t>(checksumCalcolato + byteRicevuto);
            }

            break;
        }
        case StatoRicezione::ATTESA_CHECKSUM:
        {
            bufferRx[indiceRx++] = byteRicevuto;
            timerUltimoByte = millis();

            const bool notificaCF =
                indiceRx >= 6 &&
                bufferRx[1] == 0x7F && // Mittente: Chain2 Gate
                bufferRx[2] == 0x01 && // Destinatario: Host
                bufferRx[3] == 0x4C;   // CCP_CF_NOTIFICATION

            if (byteRicevuto == checksumCalcolato)
            {
                if (notificaCF)
                {
                    trameValide++;

                    lampeggioRx = true;
                    timerLedRX = millis();

                    Serial2.write(tramaAck, sizeof(tramaAck));

                    statoRicezione = StatoRicezione::TRAMA_COMPLETA;
                }
                else
                {
                    trameIgnorate++;
                    resetRicezione();
                }
            }
            else
            {
                erroriChecksum++;

                // NACK soltanto per notifiche CF riconoscibili
                if (notificaCF)
                {
                    Serial2.write(tramaNack, sizeof(tramaNack));

                    segnalazioneErrore = true;
                    timerLedErrore = millis();
                }

                resetRicezione();
            }

            break;
        }
        case StatoRicezione::TRAMA_COMPLETA:
            break;
        }

        if (statoRicezione == StatoRicezione::TRAMA_COMPLETA)
        {
            nuoviDatiSeriali = true;
            return;
        }
    }
}

/**
 * @brief Estrae tipo CF e potenza istantanea dal buffer di ricezione.
 *
 * @details Riconosce CF21 (46 byte) e CF3 (88 byte), leggendo la potenza
 * codificata in quattro caratteri esadecimali ai rispettivi offset.
 * Azzera sempre il tipo CF; per trame non riconosciute o di lunghezza
 * inattesa mantiene il precedente valore della potenza.
 */
void parseFrame()
{
    tipoCf = 0;

    if (indiceRx < 16)
    {
        return;
    }

    const char cf1 = static_cast<char>(bufferRx[10]);
    const char cf2 = static_cast<char>(bufferRx[11]);

    if (cf1 == '1' && cf2 == '5')
    {
        if (indiceRx != 46)
        {
            return;
        }

        tipoCf = 21;

        String potenzaHex = "";

        for (int i = 36; i < 40; i++)
        {
            potenzaHex += static_cast<char>(bufferRx[i]);
        }

        potenzaIstantanea = strtoul(potenzaHex.c_str(), nullptr, 16);
    }
    else if (cf1 == '0' && cf2 == '3')
    {
        if (indiceRx != 88)
        {
            return;
        }

        tipoCf = 3;

        String potenzaHex = "";

        for (int i = 42; i < 46; i++)
        {
            potenzaHex += static_cast<char>(bufferRx[i]);
        }

        potenzaIstantanea = strtoul(potenzaHex.c_str(), nullptr, 16);
    }
}

/**
 * @brief Azzera buffer logico, checksum, timer e flag per attendere un nuovo START.
 */
void resetRicezione()
{
    indiceRx = 0;
    checksumCalcolato = 0;
    timerUltimoByte = 0;
    nuoviDatiSeriali = false;
    statoRicezione = StatoRicezione::ATTESA_STX;
}

/**
 * @brief Aggiorna i LED in base ai timer di ricezione e di errore.
 *
 * @details Il LED giallo si spegne brevemente dopo una trama valida;
 * il rosso resta acceso dopo timeout, overflow o NACK per una notifica CF.
 */
void aggiornaLed()
{
    if (lampeggioRx && millis() - timerLedRX >= durata_RX)
    {
        lampeggioRx = false;
    }

    digitalWrite(ledGiallo, lampeggioRx ? LOW : HIGH);

    if (segnalazioneErrore && millis() - timerLedErrore >= durataLedErrore)
    {
        segnalazioneErrore = false;
    }

    digitalWrite(ledRosso, segnalazioneErrore ? HIGH : LOW);
}

/**
 * @brief Mostra sul display la potenza in watt e il tipo CF riconosciuto.
 */
void aggiornaDisplay()
{
    oled.clearBuffer();

    oled.setFont(u8g2_font_6x10_tr);
    oled.drawStr(25, 10, "ChainSense2");

    oled.drawHLine(0, 14, 128);

    oled.drawStr(4, 27, "Potenza istantanea");

    char potenzaTesto[10];
    snprintf(potenzaTesto, sizeof(potenzaTesto),
             "%u", static_cast<unsigned int>(potenzaIstantanea));

    oled.setFont(u8g2_font_logisoso20_tr);
    oled.drawStr(12, 51, potenzaTesto);

    oled.setFont(u8g2_font_6x10_tr);
    oled.drawStr(106, 48, "W");

    oled.drawHLine(0, 54, 128);

    if (tipoCf == 21)
    {
        oled.drawStr(2, 63, "CF21");
    }
    else if (tipoCf == 3)
    {
        oled.drawStr(2, 63, "CF3");
    }

    oled.drawStr(90, 63, "RX OK");

    oled.sendBuffer();
}

/**
 * @brief Invia i dati in formato JSON a un client Python tramite la porta seriale.
 */
void inviaDatiPython()
{
    if (tipoCf != 21 && tipoCf != 3)
    {
        return;
    }

    Serial.print("{\"cf\":");
    Serial.print(tipoCf);

    Serial.print(",\"potenza_w\":");
    Serial.print(potenzaIstantanea);

    Serial.println("}");
}
