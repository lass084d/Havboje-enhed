#include <Wire.h>
#include <MPU6050_tockn.h>
#include <lmic.h>
#include <hal/hal.h>
#include <math.h>

// LoRa OTAA værdier
static const u1_t APPEUI[8] = {0x10, 0x99, 0x88, 0x77, 0x66, 0x56, 0x34, 0x12};
static const u1_t DEVEUI[8] = {0x69, 0x3A, 0x00, 0xD8, 0x7E, 0xD5, 0xB3, 0x70};
static const u1_t APPKEY[16] = {0xA3, 0x1D, 0xB0, 0x4A, 0x1C, 0x50, 0xC7, 0x58, 0x09, 0x6F, 0xEC, 0x75, 0x67, 0xD4, 0xC1, 0x2D};

// Spændings- og strømforbrugsmåling
const int measureVoltPin = A0;
const int currentMeasurePin = A1;
const int d1_R1 = 2200000;
const int d1_R2 = 8800000;
const float shuntResistor = 0.1;

// Lanternestatus
bool lanternOn = false;

//Array for kontrol af lanterne indenfor 24 timer:
bool lanterne[24] ={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}; 

// Tidsstyring
unsigned long startTime = 0; // Timer til fase 1 og 2
unsigned long statusSendTime = 0; // Timer til fase 3
const unsigned long checkInterval = 3600000; // 1 time (3600000 ms)
const unsigned long statusInterval = 86400000; // 24 timer (86400000 ms)
bool inCollisionBatteryCheckPhase = true; // Styrer faserne i loop

// Kollisionsdetektion
MPU6050 mpu(Wire);

// Pin mapping Dragino Shield
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

// LoRa variabler
static osjob_t sendjob;

void os_getArtEui(u1_t* buf) { memcpy(buf, APPEUI, 8); }
void os_getDevEui(u1_t* buf) { memcpy(buf, DEVEUI, 8); }
void os_getDevKey(u1_t* buf) { memcpy(buf, APPKEY, 16); }

void do_send(osjob_t* j, String message) {
    char payload[20];
    message.toCharArray(payload, 20); // Konverter String til char array
    LMIC_setTxData2(1, (uint8_t *)payload, strlen(payload), 0);
    Serial.print(F("Data sendt via LoRa: "));
    Serial.println(message);
}

float readBatteryVoltage() {
    float measuredVoltage = analogRead(measureVoltPin) * 5.0 / 1023.0;
    return measuredVoltage * (d1_R1 + d1_R2) / d1_R1;
}

float readCurrent() {
    float voltageDrop = analogRead(currentMeasurePin) * 5.0 / 1023.0;
    return voltageDrop / shuntResistor;
}

bool checkLantern() {
    float current = readCurrent();
    return current > 0.01;
}

void setup() {
    Serial.begin(9600);
    Wire.begin();
    mpu.begin();

    os_init();
    LMIC_reset();
    LMIC_setDrTxpow(DR_SF9, 14);

    startTime = millis(); // Start timeren til fase 1 og 2
    statusSendTime = millis(); // Start timeren til fase 3
    Serial.println(F("System start..."));
}

float totalAcc(){
            mpu.update();
        float accX = mpu.getAccX();
        float accY = mpu.getAccY();
        float accZ = mpu.getAccZ();
        float totalAcc = sqrt(accX * accX + accY * accY + accZ * accZ);
    return totalAcc;
}


void loop() {
    unsigned long currentTime = millis();

        // Fase 1: Kontrollér kollision og batteri konstant
        float totalAcc = totalAcc();

        // Kollisionsdetektion
        if (totalAcc > 3.0) {
            Serial.println("Kollision detekteret!");
            do_send(&sendjob, "Kollision");
        }

        // Batterispænding
        float batteryVoltage = readBatteryVoltage();
        if (batteryVoltage < 7.0) { // F.eks. tærskelværdi for lav spænding
            Serial.println("Lav batterispænding detekteret!");
            do_send(&sendjob, "Lav batterispænding");
        }

        // Skift til næste fase efter en time // Fase 2: Kontrollér lanternen én gang
        if (currentTime - startTime >= checkInterval) {
            startTime = millis(); // Genstart timeren til fase 2
        Serial.println("Tjekker lanternestatus...");
        bool lanternStatus = checkLantern();
        if (!lanternStatus) { //OPDATER DETTE; DA DEN IKKE TJEKKER DE SENESTE 24 TIMER!
            Serial.println("Lanternen har ikke lyst inden for de sidste 24 timer!");
            do_send(&sendjob, "Lanternen fejler");
        }
             // Fase 3: Send statusbesked én gang hver 24. time
    if (currentTime - statusSendTime >= statusInterval) {
        float batteryVoltage = readBatteryVoltage();
        String statusMessage = String(batteryVoltage, 1); // Batterispænding med én decimal
        Serial.print("Sender statusbesked: ");
        Serial.println(statusMessage);
        do_send(&sendjob, statusMessage);
        statusSendTime = millis(); // Genstart timeren til statusbesked
    }
    }
    os_runloop_once();
}

// Koden kører i 3 faser
// fase 1: Kontrollerer konstant for om der sker kollisioner eller om spændingen på batteriet er for lavt.
// fase 2: Efter fase 1 har kørt i 60 min skiftes der til fase 2. Denne kontrollerer om der inden for et interval på 24 er trukket en strøm fra lanternen.
// fase 3: Hvis der er gået 24 timer sendes der en statusbesked om batterispændingen.
