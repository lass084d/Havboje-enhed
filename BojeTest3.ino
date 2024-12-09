#include <Wire.h>
#include <MPU6050_tockn.h>
#include <lmic.h>
#include <hal/hal.h>
#include <math.h>
#include <SoftwareSerial.h>
#include <TinyGPSMinus.h>
//#include <avr/wdt.h>


// LoRa OTAA værdier
static const u1_t APPEUI[8] = { 0x10, 0x99, 0x88, 0x77, 0x66, 0x56, 0x34, 0x12 };
static const u1_t DEVEUI[8] = { 0x69, 0x3A, 0x00, 0xD8, 0x7E, 0xD5, 0xB3, 0x70 };
static const u1_t APPKEY[16] = { 0xA3, 0x1D, 0xB0, 0x4A, 0x1C, 0x50, 0xC7, 0x58, 0x09, 0x6F, 0xEC, 0x75, 0x67, 0xD4, 0xC1, 0x2D };

// Spændings- og strømforbrugsmåling
const int measureVoltPin = A0;

const int currentMeasurePin = 8;
boolean currentMeasurePinVal = 0;
unsigned long lastCurrentCheckTime = 0;
const unsigned long checkCurrentInterval = 10000;

const int d1_R1 = 10000000;
const int d1_R2 = 2200000;  // <-- Voltage measured over R

// Lanternestatus
bool lanternOn = false;

// 24 timer styring
uint32_t lanternData = 0b11111111111111111111111111111111;

// Tidsstyring
unsigned long startTime = 0;                   // Timer til fase 1 og 2
unsigned long statusSendTime = 0;              // Timer til fase 3
const unsigned int checkInterval = 3600000;    // 1 time (3600000 ms)
const unsigned int statusInterval = 86400000;  // 24 timer (86400000 ms)
bool inCollisionBatteryCheckPhase = true;      // Styrer faserne i loop

//Tidsstyring
unsigned long hour;
unsigned long currentHour = 0;
int hour24 = 0;
const int millisInHour = 3600000/100;


// Kollisionsdetektion
MPU6050 mpu(Wire);

// Pin mapping Dragino Shield
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 9,
  .dio = { 2, 6, 7 },
};

// LoRa variabler
static osjob_t sendjob;

// GPS opsætning
#define GPSport1 3
#define GPSport2 2
#define GPSBaud 9600
#define geoFenceRadius 10
SoftwareSerial GPSserial(GPSport1, GPSport2);
TinyGPSMinus gps;

const double EarthR = 6371.6;
bool gpsAnchor = false;
bool outOfArea = false;

struct position {
  double _lat;
  double _lon;
};
position basePos;
position currentPos;

void os_getArtEui(u1_t* buf) {
  memcpy(buf, APPEUI, 8);
}
void os_getDevEui(u1_t* buf) {
  memcpy(buf, DEVEUI, 8);
}
void os_getDevKey(u1_t* buf) {
  memcpy(buf, APPKEY, 16);
}

void do_send(osjob_t* j, String message) {
  char payload[20];
  message.toCharArray(payload, 20);  // Konverter String til char array
  LMIC_setTxData2(1, (uint8_t*)payload, strlen(payload), 0);
  Serial.print(F("Data sendt via LoRa: "));
  Serial.println(message);
}

float readBatteryVoltage(int R1, int R2) {
  int analog = analogRead(A0);
  float place = analog * ((100)/(22)) + analog;
  place = place / (1023/5);
  return place;

}
bool checkLantern() {  //Return the status of the latern (on=true) (off=false)
  unsigned long lastCurrentCheckTime = millis();
  unsigned long currentMillis = millis();
  lanternOn = false;
  while (currentMillis - lastCurrentCheckTime <= checkCurrentInterval) {
    currentMeasurePinVal = digitalRead(currentMeasurePin);
    if (currentMeasurePinVal == HIGH) {
      lanternOn = true;
      return lanternOn;
    }

    currentMillis = millis();
    delay(50);
  }
  return lanternOn;
}

float DecimalDegrees(char* x) {
  //Convert char array to float
  float decimal = atof(x);
  // The degrees are serperated then minutes is seperated
  int degrees = (int)(decimal / 100);
  float minutes = decimal - (degrees * 100);
  // decimal degrees calculated
  return degrees + (minutes / 60);
}
bool CheckPos(position pos) {
  double deltaLat = radians(pos._lat - basePos._lat);
  double deltaLon = radians(pos._lon - basePos._lon);
  double a = sin(deltaLat / 2) * sin(deltaLat / 2) + cos(radians(basePos._lat)) * cos(radians(pos._lat)) * sin(deltaLon / 2) * sin(deltaLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  double distance = EarthR * c * 1000;
  return distance >= geoFenceRadius;
}

bool isValidPos(position pos) {
  if (pos._lat != 0.0 && pos._lon != 0.0) {
    return true;
  } else {
    return false;
  }
}

position AcquireBasePos() {
  const int length = 20;
  position estimatePos[length];
  int count = 0;
  double resultLat = 0;
  double resultLon = 0;

  while (count < length) {
    if (GPSserial.available() > 0) {
      char datapoint = GPSserial.read();
      position currPos;
      bool newData = false;
      if (gps.encode(datapoint)) {
        newData = true;
      }
      if (newData) {
        currPos._lat = DecimalDegrees(gps.get_latitude());
        currPos._lon = DecimalDegrees(gps.get_longitude());
        if (isValidPos(currPos)) {
          //printPosition(currPos); // Debug formål
          estimatePos[count] = currPos;
          resultLat = resultLat + estimatePos[count]._lat;
          resultLon = resultLon + estimatePos[count]._lon;

          count++;
        }
      }
    }
    //calculate base position which is center of geofencing
    position newBasePos = { (resultLat / length), (resultLon / length) };
    gpsAnchor = true;
    return newBasePos;
  }
}

float totalAcc() {
  mpu.update();
  float accX = mpu.getAccX();
  float accY = mpu.getAccY();
  float accZ = mpu.getAccZ();
  float totalAcc = sqrt(accX * accX + accY * accY + accZ * accZ);
  return totalAcc;
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  mpu.begin();
  GPSserial.begin(GPSBaud);

  os_init();
  LMIC_reset();
  LMIC_setDrTxpow(DR_SF9, 14);

  startTime = millis();       // Start timeren til fase 1 og 2
  statusSendTime = millis();  // Start timeren til fase 3

  // basePos = AcquireBasePos(); //udkommenteret til test
  Serial.println(F("System start..."));
}




void loop() {
  unsigned long currentTime = millis();

  hour = currentTime / millisInHour;
  // Fase 1: Kontrollér kollision og batteri konstant
  float actualAcc = totalAcc();

  // Kollisionsdetektion
  if (actualAcc > 1.5) {
    Serial.println("Kollision detekteret!");
    // do_send(&sendjob, "K");
  }
  // Batterispænding
  float batteryVoltage = readBatteryVoltage(d1_R1, d1_R2);
  if (batteryVoltage < 7.0) {  // F.eks. tærskelværdi for lav spænding
    Serial.print("Lav batterispænding detekteret!  ");
    Serial.println(batteryVoltage);
    // do_send(&sendjob, "Lav batterispænding");
  }

  if (currentHour != hour) {
    Serial.println("Tjekker lanternestatus...");
    lanternData = lanternData >> 1;

    bool lanternStatus = checkLantern();
    if (lanternStatus) {
      lanternData = lanternData | 0b10000000000000000000000000000000;
    } else {
      lanternData = lanternData & 0b01111111111111111111111111111111;
    }
    if (lanternData < 255) {
      Serial.println("Lanternen har ikke lyst inden for de sidste 24 timer!");
      // do_send(&sendjob, "L");
    }
    /*
    currentPos = AcquireBasePos();
    outOfArea = CheckPos(currentPos);
    if (outOfArea) {
      Serial.println("Buoy er uden for område!");
      // do_send(&sendjob, "P");
    }
    */
    currentHour = hour;
    hour24++;

    if (hour24 == 24) {
      float batteryVoltage = readBatteryVoltage(d1_R1, d1_R2);
      String statusMessage = String(batteryVoltage, 1);  // Batterispænding med én decimal
      Serial.print("Sender statusbesked: ");
      Serial.println(statusMessage);
      // do_send(&sendjob, statusMessage);
      statusSendTime = millis();  // Genstart timeren til statusbesked
      hour24 = 0;
    }
  }
  os_runloop_once();
}
