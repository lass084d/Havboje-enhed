//ÆNDRING AF BRANCH
// variables for voltage measurement;
const int d1_R1 = 2200000; // resistor name: divider#nr_R#nr
const int d1_R2 = 8800000;
const int measureVolt_analogPin = 0;

// variables for current measurement
const int current_R1 = 220; // resistor used to measure current;
const unsigned long checkInterval = 10000; //10 sekunder
const unsigned long lastCheckTime = 0;
bool lanternOn = false;


//Get the battery voltage, measured using the voltage divider circuit;
float readVoltage(int R1, int R2, int analogPin) {
  // analogRead values =(0-1023)=(0V-5V), coresponding to 204.6 "point" pr. 1V voltage increase;
  float measuredVoltage = analogRead(analogPin) / (1023/5);
  float realVoltage = measuredVoltage * (R2/R1) + measuredVoltage;
  return realVoltage;
};


//Get the current through the lantern
float readCurrent(int R1) {
  //Get the voltage over current_R1 by the difference of the battery and lantern voltage
  float voltDiff = readVoltage(d1_R1,d1_R2,0) - readVoltage(d1_R1,d1_R2,1); //battery voltage =pin0 & lantern voltage =pin1
  float current = (voltDiff / current_R1);
  return current;
}


// Check the current status of the lantern
bool checkLantern (){ //Return the status of the latern (on=true) (off=false)
  unsigned long lastCheckTime = millis();
  unsigned long currentMillis = millis();
  lanternOn = false;


  while (currentMillis - lastCheckTime <= checkInterval) {
    if (readCurrent(current_R1) > 0.01) {
      lanternOn= true;
      return lanternOn;
    }
    
    currentMillis = millis();
    delay(50);
  }
  return lanternOn;
}


void setup() {
}

void loop() {
}
