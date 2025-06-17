#include <Servo.h>
#include <SoftwareSerial.h>

// Servo motor
Servo servo1;

// SIM800L GSM module setup
SoftwareSerial sim800l(2, 3); // TX, RX (Connect SIM800L TX to D2, RX to D3)

// Pin definitions
const int irPin = A1;         // IR sensor for detecting waste
const int potPin = A0;        // Soil moisture sensor
const int trigDryPin = 4;     // Ultrasonic for dry waste
const int echoDryPin = 5;
const int trigWetPin = 6;     // Ultrasonic for wet waste
const int echoWetPin = 7;

// Variables
int soil = 0;
int fsoil = 0;
bool smsSent = false;  // Flag to avoid multiple SMS alerts

void setup() {
  Serial.begin(9600);
  sim800l.begin(9600); // GSM module baud rate

  // Servo setup
  servo1.attach(8);
  servo1.write(90);

  // Pin modes
  pinMode(irPin, INPUT);
  pinMode(trigDryPin, OUTPUT);
  pinMode(echoDryPin, INPUT);
  pinMode(trigWetPin, OUTPUT);
  pinMode(echoWetPin, INPUT);

  // Initialize SIM800L
  delay(1000);
  sendCommand("AT"); // Check GSM module
  sendCommand("AT+CMGF=1"); // Set SMS mode
}

void loop() {
  // Read IR sensor
  int objectDetected = digitalRead(irPin);

  if (objectDetected == LOW) {  // Object detected
    delay(1000);

    // Read soil moisture
    fsoil = 0;
    for (int i = 0; i < 3; i++) {
      soil = analogRead(potPin);
      soil = constrain(soil, 485, 1023);
      fsoil += map(soil, 485, 1023, 100, 0);
      delay(75);
    }
    fsoil /= 3;

    // Control servo
    if (fsoil > 3) {
      servo1.write(180); // Open wet waste section
    } else {
      servo1.write(0); // Open dry waste section
    }

    delay(3000);
    servo1.write(90); // Return to neutral position
  }

  // Read ultrasonic sensors
  int dryDistance = readUltrasonicLevel(trigDryPin, echoDryPin);
  int wetDistance = readUltrasonicLevel(trigWetPin, echoWetPin);

  // Calculate percentage levels
  int dryLevelPercent = calculateDustbinLevelPercentage(dryDistance);
  int wetLevelPercent = calculateDustbinLevelPercentage(wetDistance);

  // Send data to ESP8266 via Serial
  Serial.print("Soil:");
  Serial.print(fsoil);
  Serial.print(",Dry:");
  Serial.print(dryLevelPercent);
  Serial.print(",Wet:");
  Serial.println(wetLevelPercent);

  // Send SMS alert when bin is full (only once)
  if ((dryLevelPercent == 100 || wetLevelPercent == 100) && !smsSent) {
    sendSMS("Smart Dustbin Alert: The bin is FULL! Please empty it.");
    smsSent = true;
  }

  // Reset SMS flag when bin is emptied
  if (dryLevelPercent < 100 && wetLevelPercent < 100) {
    smsSent = false;
  }

  delay(1000); // General delay
}

// Function to read ultrasonic sensor distance
int readUltrasonicLevel(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2; // Convert to cm
  return distance;
}

// Function to calculate dustbin level percentage
int calculateDustbinLevelPercentage(int distance) {
  if (distance > 20) return 0;
  if (distance >= 16 && distance <= 18) return 25;
  if (distance >= 9 && distance <= 12) return 50;
  if (distance >= 5 && distance <= 7) return 75;
  if (distance >= 1 && distance <= 2) return 100;
  return 0;
}

// Function to send AT commands to SIM800L
void sendCommand(String command) {
  sim800l.println(command);
  delay(1000);
  while (sim800l.available()) {
    Serial.write(sim800l.read());
  }
}

// Function to send SMS alert
void sendSMS(String message) {
  Serial.println("Sending SMS...");
  sim800l.println("AT+CMGS=\"+918217388471\""); // Replace with recipient number
  delay(1000);
  sim800l.println(message);
  delay(1000);
  sim800l.write(26); // End of message
  delay(3000);
  Serial.println("SMS Sent!");
}