int resistance = 220;
unsigned long checkInterval = 10000; //10 sekunder
unsigned long lastCheckTime = 0;
bool lanternOn = false;

void setup() {
  Serial.begin(9600);
  bool lanterneStatus = CheckLaterne();

  if (lanterneStatus) {
    Serial.println("Lanterne Lyser");

  } else {
    Serial.println("Lanterne Lyser ikke");
  }
}
void loop() {
  
}

bool CheckLaterne (){ //Return the status of the latern (on=true) (off=false)
  unsigned long lastCheckTime = millis();
  unsigned long currentMillis = millis();
  lanternOn = false;


  while (currentMillis - lastCheckTime <= checkInterval) {
    int sensorValue1 = analogRead(A0);
    int sensorValue2 = analogRead(A1);

    float voltDiff = sensorValue1 * (5.0 / 1023.0) - sensorValue2 * (5.0 / 1023.0);
    float current = (voltDiff / resistance);
    Serial.println(current);


    if (current > 0.01) {
      lanternOn= true;
      return lanternOn;
    }
    
    currentMillis = millis();
    delay(50);
  }
  return lanternOn;
}