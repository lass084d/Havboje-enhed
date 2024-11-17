// Inkluderer LMIC-biblioteket til LoRaWAN-protokollen og HAL (Hardware Abstraction Layer)
#include <lmic.h>
#include <hal/hal.h>

// OTAA værdier (Over-the-Air Activation) for at autentificere enheden på LoRaWAN-netværket
static const u1_t APPEUI[8] = {0x10, 0x99, 0x88, 0x77, 0x66, 0x56, 0x34, 0x12}; // Application EUI (identificerer applikationen)
static const u1_t DEVEUI[8] = {0x69, 0x3A, 0x00, 0xD8, 0x7E, 0xD5, 0xB3, 0x70}; // Device EUI (unik enheds-ID)
static const u1_t APPKEY[16] = {0xA3, 0x1D, 0xB0, 0x4A, 0x1C, 0x50, 0xC7, 0x58, 0x09, 0x6F, 0xEC, 0x75, 0x67, 0xD4, 0xC1, 0x2D}; // Krypteringsnøgle

// Callback-funktioner, der leverer ovenstående nøgler til LMIC, når de bliver efterspurgt
void os_getArtEui (u1_t* buf) { memcpy(buf, APPEUI, 8); }  // APPEUI overføres til LMIC-bufferen
void os_getDevEui (u1_t* buf) { memcpy(buf, DEVEUI, 8); }  // DEVEUI overføres til LMIC-bufferen
void os_getDevKey (u1_t* buf) { memcpy(buf, APPKEY, 16); } // APPKEY overføres til LMIC-bufferen

// Opretter en jobstruktur til planlagte opgaver som at sende data
static osjob_t sendjob;

// Definerer intervallet mellem transmissioner (660 sekunder = 11 minutter)
const unsigned TX_INTERVAL = 660;

// Pin-konfiguration for Dragino LoRa Shield
const lmic_pinmap lmic_pins = {
    .nss = 10,                // SPI "chip select" pin
    .rxtx = LMIC_UNUSED_PIN,  // Ikke brugt
    .rst = 9,                 // Reset pin
    .dio = {2, 3, 4},         // Digital I/O pins for LoRa-modulet
};

// Callback-funktion, der håndterer LoRaWAN-begivenheder
void onEvent (ev_t ev) {
    if (ev == EV_TXCOMPLETE) {  // Når en transmission er færdig
        Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));

        // Planlægger næste transmission efter TX_INTERVAL
        os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    }
}

// Funktion til at sende data via LoRaWAN
void do_send(osjob_t* j){
    // Data, der skal sendes (payload) – i dette tilfælde "hi" i tekstform
    static uint8_t message[] = {0x68, 0x69};  // Hexadecimal repræsentation af "hi"

    // Checker, om der allerede er en igangværende transmission
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending")); // Hvis der allerede sendes, sendes der ikke nyt
    } else {
        // Forbered transmission af data
        LMIC_setTxData2(1, message, sizeof(message), 0); // Port 1, payload, længde, uacknowledged
        Serial.println(F("Sending uplink packet..."));  // Debug output
    }
}

// Setup-funktionen initialiserer alt og kører én gang ved opstart
void setup() {
    Serial.begin(115200);  // Starter seriekommunikation for debug-output
    Serial.println(F("Starting..."));

    // Initialiserer LMIC-biblioteket
    os_init();

    // Nulstiller LMIC, så tidligere sessioner og data slettes
    LMIC_reset();

    // Starter første transmission med do_send
    do_send(&sendjob);
}

// Loop-funktionen kører kontinuerligt og håndterer LMIC's baggrundsopgaver
void loop() {
    os_runloop_once(); // Kører én iteration af LMIC's eventloop
}