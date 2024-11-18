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
bool gpsAnchor;
bool outOfArea;
int geoFenceRadius = 10; // allowed movement in m before alarmtriggering

struct position{
  double _lat;
  double _lon;
};
position basePos;
position currentPos;

void setup() {
  // put your setup code here, to run once:


  Serial.begin(SerialBaud);
  GPSserial.begin(GPSBaud);
  gpsAnchor = false;

}

void loop() {
  // put your main code here, to run repeatedly:
  

  if (gpsAnchor == false){
    basePos = AcquireBasePos();
    Serial.print("BasePos:");
    printPosition(basePos);
  }else{
    //currentPos = AcquireBasePos();
    currentPos = GetPos();
    //printPosition(currentPos); // debug
    outOfArea = CheckPos(currentPos);
    Serial.println(outOfArea);
  }
  
}

position GetPos(){
  position currPos;
  bool newData = false;
  while(!newData){
  if (GPSserial.available() > 0){
    char datapoint = GPSserial.read();

    if (gps.encode(datapoint)) {
      newData = true;
    }
  }
    if (newData){
      currPos._lat = DecimalDegrees(gps.get_latitude());
      currPos._lon = DecimalDegrees(gps.get_longitude());
      //printPosition(currPos);
      if(isValidPos(currPos)){
        return currPos;

      }
    }
  }
}


//Generates the anchor point for the bouy
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
      if(newData){
        currPos._lat = DecimalDegrees(gps.get_latitude());
        currPos._lon = DecimalDegrees(gps.get_longitude());
        if(isValidPos(currPos)){
          //printPosition(currPos); // Debug formål
          estimatePos[count] = currPos;
          resultLat = resultLat + estimatePos[count]._lat;
          resultLon = resultLon + estimatePos[count]._lon;

          count++;
        }

      }
    }
  }
  
  // Debug: Print hele arrayet
  printPosArr(estimatePos, length);
  //calculate base position which is center of geofencing 
  position newBasePos = {(resultLat/length), (resultLon/length)};
  gpsAnchor = true;
  return newBasePos;

}

//double CalcEstimate{}

//Parses from degrees and minutes (ddmm.mmmm) to degrees in decimalform
float DecimalDegrees(char* x){
  //Convert char array to float
  float decimal = atof(x);
  // The degrees are serperated then minutes is seperated
  int degrees = (int)(decimal/100);
  float minutes = decimal - (degrees * 100);
  // decimal degrees calculated
  return degrees + (minutes / 60);
  
}

bool isValidPos(position pos){
  if(pos._lat != 0.0 && pos._lon != 0.0){
    return true;
  }else {
    return false;
  }
}

// calculates the distance between an input position and the configured anchorpoint and evaluate if driftet
bool CheckPos(position pos){
  // converts to decimaldegrees
  //float latitude = DecimalDegrees(_latitude);
  //float longitude = DecimalDegrees(_longitude);
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
  double a = sin(deltaLat / 2) * sin(deltaLat / 2) + cos(anchorLatRad) * cos(latitudeRad) * sin(deltaLon/2) * sin(deltaLon/2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  double distance = EarthR * c * 1000; //convert to meters when calculating distance from center 1000 because earth radius is in km

  Serial.print("Dist from center: ");
  Serial.println(distance, 6);
  return distance >= geoFenceRadius;// returns false when distance from center is longer than the defined geofence radius 

}

void printPosition(position pos){
  Serial.print(pos._lat, 8);
  Serial.print(", ");
  Serial.println(pos._lon, 10);
}

void printPosArr(position posArr[], int length){
  Serial.println("---------------");
  for (int i = 0 ; i < length; i++){
    Serial.print(i);
     Serial.print(": ");
    printPosition(posArr[i]);
  }
}
