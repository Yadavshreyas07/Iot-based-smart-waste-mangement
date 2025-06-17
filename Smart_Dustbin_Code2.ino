#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <ESP_Mail_Client.h>

// WiFi Credentials
const char* ssid = "RJGP";          
const char* password = "1234567809"; 

// ThingSpeak API Key and Channel ID
unsigned long channelID = 2789114; 
const char* writeAPIKey = "55R2FABC0OSCKWYL"; 

WiFiClient client;

// SMTP Email Credentials
#define AUTHOR_EMAIL "arvishramanthrk@gmail.com"
#define AUTHOR_PASSWORD "vmlq bscx jwoz qdpd" // Use an App Password!
#define RECIPIENT_EMAIL "shreyask3002@gmail.com"

// SMTP Configuration
SMTPSession smtp;
ESP_Mail_Session session;
SMTP_Message message;

void setup() {
    Serial.begin(9600);
    connectToWiFi();
    ThingSpeak.begin(client);
    setupEmail();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectToWiFi();
    }

    if (Serial.available()) {
        String data = Serial.readStringUntil('\n');
        data.trim();
        if (data.length() > 0) {
            processAndSendData(data);
        }
    }

    delay(15000); // Send data every 15 seconds
}

// Function to process data and send to ThingSpeak and email alert
void processAndSendData(String data) {
    int soilMoisture = extractValue(data, "Soil");
    int dryLevelPercent = extractValue(data, "Dry");
    int wetLevelPercent = extractValue(data, "Wet");

    Serial.println("Uploading to ThingSpeak...");
    Serial.print("Soil Moisture: "); Serial.println(soilMoisture);
    Serial.print("Dry Waste Level: "); Serial.println(dryLevelPercent);
    Serial.print("Wet Waste Level: "); Serial.println(wetLevelPercent);

    ThingSpeak.setField(1, soilMoisture);
    ThingSpeak.setField(2, dryLevelPercent);
    ThingSpeak.setField(3, wetLevelPercent);

    int response = ThingSpeak.writeFields(channelID, writeAPIKey);

    if (response == 200) {
        Serial.println("✅ Data successfully sent to ThingSpeak!");
    } else {
        Serial.print("❌ Failed to send data. HTTP Error: ");
        Serial.println(response);
    }

    // Send an email if any waste level reaches 100%
    if (dryLevelPercent >= 100 || wetLevelPercent >= 100) {
        sendEmailAlert(dryLevelPercent, wetLevelPercent);
    }
}

// Function to extract values from the received data
int extractValue(String data, String key) {
    int keyIndex = data.indexOf(key + ":");
    if (keyIndex == -1) return 0;

    int valueStart = keyIndex + key.length() + 1;
    int valueEnd = data.indexOf(',', valueStart);
    if (valueEnd == -1) valueEnd = data.length();

    return data.substring(valueStart, valueEnd).toInt();
}

// Function to connect to WiFi
void connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Connected to WiFi!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ Failed to connect to WiFi. Retrying...");
    }
}

// Function to setup SMTP email
void setupEmail() {
    session.server.host_name = "smtp.gmail.com";
    session.server.port = 465;  // SSL port
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD; // Use Gmail App Password
    session.login.user_domain = "localhost";

    smtp.debug(1);  // Enable debugging messages
    smtp.callback(smtpCallback);
}

// Function to send an email alert
void sendEmailAlert(int dryLevel, int wetLevel) {
    Serial.println("🚨 Sending email alert...");

    String emailBody = "⚠ Waste Bin Alert!\n\n";
    if (dryLevel >= 100) emailBody += "🚨 Dry Waste bin is FULL.\n";
    if (wetLevel >= 100) emailBody += "🚨 Wet Waste bin is FULL.\n";

    message.sender.name = "Smart Dustbin";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "Waste Bin Full Alert!";
    message.addRecipient("User", RECIPIENT_EMAIL);
    message.text.content = emailBody.c_str();

    if (!smtp.connect(&session)) {
        Serial.println("❌ SMTP connection failed.");
        return;
    }

    if (MailClient.sendMail(&smtp, &message, true)) {
        Serial.println("✅ Email sent successfully!");
    } else {
        Serial.print("❌ Email failed: ");
        Serial.println(smtp.errorReason());
    }
}

// Callback function for SMTP debugging
void smtpCallback(SMTP_Status status) {
    Serial.println(status.info());
}