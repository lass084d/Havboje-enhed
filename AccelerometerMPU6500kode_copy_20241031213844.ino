// Bibliotek til at bruge I2C protokollen og til at kommunikere med accelerometeret
#include <Wire.h>
#include <MPU6050_tockn.h>

MPU6050 mpu(Wire);

void setup() {
  Serial.begin(115200);
  Wire.begin(); // Initialiserer I2C-protokollen.
  mpu.begin(); // Starter MPU6050-sensoren
}

void loop() {
  mpu.update(); // Opdaterer målingerne fra sensoren
  
  // Disse linjer henter accelerationsdata for X-, Y- og Z-aksen fra sensoren.
  float accX = mpu.getAccX();
  float accY = mpu.getAccY();
  float accZ = mpu.getAccZ();
  
  // Beregning den samlede acceleration
  float totalAcc = sqrt(accX * accX + accY * accY + accZ * accZ);
  
  // Tjek om accelerationen overstiger 3g
  if (totalAcc > 1.0) {
    Serial.println("Kollision registreret");
  }
  
  delay(100);
}
