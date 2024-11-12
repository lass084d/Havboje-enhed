// variables for voltage measurement;
int d1_R1 = 2200000 // resistor name: divider#nr_R#nr
int d1_R2 = 8800000
int measureVolt_analogPin = 0;

//Get the battery voltage, measured using the voltage divider circuit;
float readVoltage(int R1, int R2, int analogPin) {
  // analogRead values =(0-1023)=(0V-5V), coresponding to 204.6 "point" pr. 1V voltage increase;
  float measuredVoltage = analogRead(analogPin) / (1023/5);
  float realVoltage = measuredVoltage * (R2/R1) + measuredVoltage;
  return realVoltage;
};



void setup() {
}

void loop() {
}

