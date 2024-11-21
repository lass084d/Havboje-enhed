// MIT License
// https://github.com/gonzalocasas/arduino-uno-dragino-lorawan/blob/master/LICENSE
// Baseret på eksempler fra https://github.com/matthijskooijman/arduino-lmic
// Copyright (c) 2015 Thomas Telkamp og Matthijs Kooijman

// Tilpasninger: Andreas Spiess
#include <lmic.h>
#include <hal/hal.h>

// OTAA værdier til LoRa modul - APPEUI, DEVEUI og APPKEY bruges til at identificere og sikre forbindelsen.
static const u1_t APPEUI[8] = {0x10, 0x99, 0x88, 0x77, 0x66, 0x56, 0x34, 0x12};
static const u1_t DEVEUI[8] = {0x69, 0x3A, 0x00, 0xD8, 0x7E, 0xD5, 0xB3, 0x70};
static const u1_t APPKEY[16] = {0xA3, 0x1D, 0xB0, 0x4A, 0x1C, 0x50, 0xC7, 0x58, 0x09, 0x6F, 0xEC, 0x75, 0x67, 0xD4, 0xC1, 0x2D};

// Callbacks til at hente OTAA-nøgler - sender APPEUI, DEVEUI og APPKEY til netværksmodulet.
void os_getArtEui (u1_t* buf) { memcpy(buf, APPEUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy(buf, DEVEUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy(buf, APPKEY, 16); }

static osjob_t sendjob; // Variabel til at håndtere tidsplanlagte opgaver

// Interval for dataoverførsel (i sekunder)
const unsigned TX_INTERVAL = 100;

// Pin-konfiguration til Dragino Shield - specificerer hvilke pins, der bruges.
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

// Eventhåndtering - denne funktion kaldes, når der sker bestemte begivenheder.
void onEvent (ev_t ev) {
    if (ev == EV_TXCOMPLETE) { // Tjekker om transmissionen er afsluttet
        Serial.println(F("EV_TXCOMPLETE (inkluderer ventetid på RX vinduer)"));
        // Planlæg næste transmission
        os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    }
}

// Funktion til at sende data
void do_send(osjob_t* j) {
    // Data (payload) der skal sendes
    static uint8_t message[] = "hio"; // Data-pakke til uplink

    // Tjekker, om der allerede kører en TX/RX-job
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, sender ikke"));
    } else {
        // Forbered data til næste mulige transmission
        LMIC_setTxData2(1, message, sizeof(message)-1, 0);
        Serial.println(F("Sender uplink-pakke..."));
    }
    // Næste transmission planlægges efter TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200); // Starter seriel kommunikation
    Serial.println(F("Starter..."));

    os_init(); // Initialiserer LMIC-biblioteket
    LMIC_reset(); // Nulstiller MAC-status og afbryder evt. ventende dataoverførsler

    // Sætter spredningsfaktoren til SF9 og sendeeffekten til 14 dBm
    LMIC_setDrTxpow(DR_SF9, 14); 

    // Starter det første send-job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once(); // Kører én iteration af LMIC's hovedløkkefunktion
}
