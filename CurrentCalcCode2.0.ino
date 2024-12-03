unsigned long checkInterval = 10000; //10 sekunder
unsigned long lastCheckTime = 0;
bool lanternOn = false;
int inPin = 7; 
int inPinVal = 0; 

void setup() {
  Serial.begin(9600);
  pinMode(inPin, INPUT); 

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
  inPinVal = digitalRead(inPin); 
    if (inPinVal == HIGH) {
      lanternOn= true;
      return lanternOn;
    }
    
    currentMillis = millis();
    delay(50);
  }
  return lanternOn;
}