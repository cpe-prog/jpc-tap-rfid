#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Define pins for RFID and SPI
#define SS_PIN D2
#define RST_PIN D1

// Define WiFi credentials
#define WIFI_SSID "I'm in!"
#define WIFI_PASSWORD "connected"

// Define Firebase credentials
#define API_KEY "AIzaSyCcvpVGRyAqj5bRAZjD8Z0ge0lOrbzuHTw"
#define DATABASE_URL "jpctap-dfd46-default-rtdb.firebaseio.com/" 

// Create an instance of the MFRC522 class
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Create instances for Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

byte nuidPICC[4];

void setup() {
  Serial.begin(115200);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  Serial.println();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println();
  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    if (Firebase.signUp(&config, &auth, "", "")){
      Serial.println("ok");
      signupOK = true;
    }
    else{
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    config.token_status_callback = tokenStatusCallback;
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void loop() {
  // Check if Firebase is ready and sign-up is successful
 if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    // Look for new cards
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
      return;
    }
    // Read the RFID tag UID
    String currentRFID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      currentRFID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      currentRFID += String(rfid.uid.uidByte[i], HEX);
    }
    currentRFID.toUpperCase();
    Serial.println("RFID: " + currentRFID);
    if (Firebase.RTDB.setString(&fbdo, "Payment/1/rfid", currentRFID)) {
      Serial.println("Data sent successfully.");
      Serial.println("RFID: " + currentRFID);
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
