#define swsRX 3
#define swsTX 2

//GPS Baudrate
#define GPSBaud 9600

//Serial Monitor Baud
#define SerialBaud 9600

//include internal lib
#include <SoftwareSerial.h>
#include <TinyGPSMinus.h>
#include <math.h>
SoftwareSerial GPSserial(swsRX, swsTX);
TinyGPSMinus gps;

const double EarthR = 6371.6;
bool outOfArea;
int geoFenceRadius = 10;  // allowed movement in m before alarmtriggering

struct position {
  double _lat;
  double _lon;
};
position basePos;
position currentPos;

void setup() {
  // put your setup code here, to run once:see3e3e3eeee333ee3eeeeeeeee
  Serial.begin(SerialBaud);
  GPSserial.begin(GPSBaud);
  basePos = GetPos();
  Serial.print("BasePos:");
  printPosition(basePos);
}

void loop() {
  // put your main code here, to run repeatedly:
  //currentPos = AcquireBasePos();
  delay(40000);
  Serial.println("Start nye målinger");
  currentPos = GetPos();
  //printPosition(currentPos); // debug
  outOfArea = CheckPos(currentPos);
  Serial.println(outOfArea);
}

position GetPos() {
  position currPos;
  int i= 0;
  int length = 50;
  double estimatePosLat = 0;
  double estimatePosLon = 0;

  while (i < length) {
    bool newData = false;
    while (!newData) {
      if (GPSserial.available() > 0) {
        char datapoint = GPSserial.read();

        if (gps.encode(datapoint)) {
          newData = true;
        }
      }
      if (newData) {
        currPos._lat = DecimalDegrees(gps.get_latitude());
        currPos._lon = DecimalDegrees(gps.get_longitude());
        //printPosition(currPos);
        if (isValidPos(currPos)) {
          estimatePosLat += currPos._lat;
          estimatePosLon += currPos._lon;
          i++;
        }
      }
    }
  }
  currPos._lat = estimatePosLat/length;
  currPos._lon = estimatePosLon/length;
  return currPos;
}

//Parses from degrees and minutes (ddmm.mmmm) to degrees in decimalform
float DecimalDegrees(char* x) {
  //Convert char array to float
  float decimal = atof(x);
  // The degrees are serperated then minutes is seperated
  int degrees = (int)(decimal / 100);
  float minutes = decimal - (degrees * 100);
  // decimal degrees calculated
  return degrees + (minutes / 60);
}

bool isValidPos(position pos) {
  if (pos._lat != 0.0 && pos._lon != 0.0) {
    return true;
  } else {
    return false;
  }
}

// calculates the distance between an input position and the configured anchorpoint and evaluate if driftet
bool CheckPos(position pos) {
  //converts current pos to radians
  double latitudeRad = radians(pos._lat);
  double longitudeRad = radians(pos._lon);
  //converts anchor (mby move to generateAnchorpos)
  double anchorLatRad = radians(basePos._lat);
  double anchorLonRad = radians(basePos._lon);

  printPosition(basePos);
  printPosition(pos);

  //Using Haversine formula to calculate distamce between and evaluate if bouy has moved outside designate area
  double deltaLat = latitudeRad - anchorLatRad;
  double deltaLon = longitudeRad - anchorLonRad;
  double a = sin(deltaLat / 2) * sin(deltaLat / 2) + cos(anchorLatRad) * cos(latitudeRad) * sin(deltaLon / 2) * sin(deltaLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  double distance = EarthR * c * 1000;  //convert to meters when calculating distance from center 1000 because earth radius is in km

  Serial.print("Dist from center: ");
  Serial.println(distance, 6);
  return distance >= geoFenceRadius;  // returns false when distance from center is longer than the defined geofence radius
}

void printPosition(position pos) {
  Serial.print(pos._lat, 8);
  Serial.print(", ");
  Serial.println(pos._lon, 10);
}
