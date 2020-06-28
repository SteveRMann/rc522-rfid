#define SKETCH_NAME "Monkey_Detector.ino"
#define SKETCH_VERSION "V1.5"
#define HOSTPREFIX "monkey"


/*
   ----------------------------------------------------------------------
   This sketch reads RFID cards or tags then publishes the ID code over MQTT
   using the topic of "monkey/msg"

   Board: NodeMCU 1.0 (ESP-12E Module)

   ----------------------------------------
   RC522 Interfacing with NodeMCU
   Pin layout used (Wemos not tested):
   MFRC522      NodeMCU           Wemos D1 Mini
   ----------------------------------------
   SDA          D2 (GPIO4)        D8
   SCK          D5 (GPIO14)       D5
   MOSI         D7 (GPIO13)       D7
   MISO         D6 (GPIO12)       D6
   IRQ          NC
   GND          GND
   RST          D1 (GPIO5)        D3
   3.3V         3.3V
   ----------------------------------------
   Version 1.2  Added read/write the servo positions to EEPROM.
   Version 1.3  Added cmnd functions:
                LUT 83,104,3000       //Set servo (lock, unlock) positions and drawer time in ms
                status                //Returns servo positions and drawer time in /monkey/status
                tune                  //Start lock-unlock in one-second cycles
                reset                 //Turns off tune and unlicks drawer for drawerTime ms.
   Version 1.4  Removed 'delay' statements from the tune section in loop
   Version 1.5  Any card/tag will unlock the drawer.
                cmnd lut triggers an mqtt status update
                Open with a card/tag detected will open/close rapidly a few times.
                Added 'lockStatus' to be reported with each cmnd/status.

*/


// ===== wifi vars =====
char macBuffer[24];       // Holds the last three digits of the MAC, in hex.
char hostNamePrefix[] = HOSTPREFIX;
char hostName[24];        // Holds hostNamePrefix + the last three bytes of the MAC address.

#define LED_ON HIGH
#define LED_OFF LOW

#define LOCKED HIGH
#define UNLOCKED LOW
bool tuneState = false;
bool lockStatus;
bool tuneFlag = false;


// ===== timers =====
unsigned long drawerMillis;           // Used to time how long the drawer stays unlocked.
unsigned long tuneMillis;             // Timer for tuning
unsigned long tuneTime = 1000;        // Tune duration in ms
unsigned long ledMillis;              //LED ON timer
unsigned long ledTime = 300;          //LED ON duration


#include <SPI.h>
#include <MFRC522.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "D:\River Documents\Arduino\libraries\Kaywinnet.h"
#include <EEPROM.h>
#include <Servo.h>
Servo servo;

#include "eepromAnything.h"

struct servo {
  int lockedPosition ;
  int unlockedPosition ;
  unsigned long drawerTime;
};
struct servo myServo;
int lockedPosition = myServo.lockedPosition;
int unlockedPosition = myServo.unlockedPosition;
unsigned long drawerTime = myServo.drawerTime;



//Declare an object of class WiFiClient, which allows to establish a connection to a specific IP and port.
//We will also declare an object of class PubSubClient, which receives as input of the constructor the previously defined WiFiClient.
//
// Initialize the espClient. The espClient name must be unique
WiFiClient Monkey1;
PubSubClient client(Monkey1);


#ifndef mqtt_server
#define mqtt_server "192.168.1.124"
#endif
//#define client_name hostName           //Must be unique

char msgIN;

const char *statusTopic = HOSTPREFIX "/status";
//const char *connectName =  HOSTPREFIX "xyzz1Monkey";             //Must be unique
const char* msgTopic = HOSTPREFIX "/msg";
const char* cmdTopic = HOSTPREFIX "/cmnd";
const char* lockTopic = HOSTPREFIX "/l";
const char* unlockTopic = HOSTPREFIX "/u";
const char* drawerTimeTopic = HOSTPREFIX "/t";


//const int unlockCard1 = 627;         // This card number unlocks the drawer
//const int unlockCard2 = 366;         // This card number unlocks the drawer


// Pins and variables
const int servoPin = D4;                //GPIO2
const int unlockSwitchPin = D8;         //Switches 3V3 to D8, D8 pulled down.
const int LEDPIN = 10;                  //NodeMCU SD3 (GPIO10). Blink this LED when a new card is detected.
int unlockSwitchValue;


//MFRC pins
constexpr uint8_t RST_PIN = 5;      // D1 on the NodeMCU
constexpr uint8_t SS_PIN = 4;

MFRC522 rfid(SS_PIN, RST_PIN);      // Create an instance of the class, cal it rfid
MFRC522::MIFARE_Key key;

byte nuidPICC[4];                   // Init the array that will store new Tag ID (nuid)
int cardId = 0;                     // Card ID (Sum of NUID byes)









//============================= Setup =============================
void setup() {
  beginSerial();
  EEPROM.begin(40);                      //holds 2 integers and one long int.
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println(F("****************"));
  Serial.print(F("msgTopic= "));
  Serial.println(msgTopic);
  Serial.print(F("cmdTopic= "));
  Serial.println(cmdTopic);
  Serial.print(F("statusTopic= "));
  Serial.println(statusTopic);
  Serial.print(F("lockTopic= "));
  Serial.println(lockTopic);
  Serial.print(F("unlockTopic= "));
  Serial.println(unlockTopic);
  Serial.print(F("drawerTimeTopic= "));
  Serial.println(drawerTimeTopic);
  Serial.println(F("****************"));

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.println();
  Serial.print(F("MAC Address: "));
  Serial.println(macToStr(mac));
  Serial.println();

  EEPROM_readAnything(0, myServo);
  lockedPosition = myServo.lockedPosition;
  unlockedPosition = myServo.unlockedPosition;
  drawerTime = myServo.drawerTime;

  //Defaults
  if (lockedPosition < 0) lockedPosition = 85;
  if (unlockedPosition < 0) unlockedPosition = 103;
  if (drawerTime > 45000) drawerTime = 1000;

  showStatus();

  Serial.print(F("LOCKED= "));
  Serial.println(LOCKED);
  Serial.print(F("UNLOCKED= "));
  Serial.println(UNLOCKED);
  Serial.println();

  SPI.begin();                    // Init SPI bus

  //Init the MFRC522
  delay(100);
  rfid.PCD_Init();                // Init MFRC522
  rfid.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader
  delay(100);
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println(F("Scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();
  Serial.println();


  pinMode(LEDPIN, OUTPUT);
  pinMode(unlockSwitchPin, INPUT);  //You can't use input_pullup on pins 15 (D8) and 16 (D0).
  servo.attach(servoPin);

  //Start with the servo in the unlocked position for 5-seconds
  lockStatus = UNLOCKED;           //Not initialized yet
  drawerUnlock();
  delay(5000);
  drawerLock();


}




//============================= Loop =============================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  //delay(100);             // Adds stability to MQTT connection.
  client.loop();



  // ===== tune =====
  if (tuneFlag == true) {
    // Cycle through lock/unlock every tuneTime ms.
    if (millis() - tuneMillis > tuneTime) {       //Have we been locked or unlocked a while?
      tuneMillis = millis();                      //  Yes, restart the tune cycle
      tuneState = !tuneState;                     //  Toggle the lock state.

      if (tuneState == LOCKED) {                  //If the new tuneState is locked,
        //Serial.println(F("Locked"));
        servo.write(lockedPosition);              //  lock the drawer.
      }
      if (tuneState == UNLOCKED) {
        //Serial.println(F("Unlocked"));
        servo.write(unlockedPosition);
      }
    }
    return;
  }



  // ===== Read the Unlock Switch value (D8) =====
  unlockSwitchValue = digitalRead(unlockSwitchPin);
  if (unlockSwitchValue == HIGH) {
    drawerUnlock();
    drawerMillis = millis();                       //Start a timer to lock it again
  }

  if (millis() - drawerMillis > drawerTime) {     //If the drawer has been unlocked for a while, lock it.
    drawerLock();
  }

  if (millis() - ledMillis > ledTime) {          //If the LED has been on for a while, turn it off.
    digitalWrite(LEDPIN, LED_OFF);
  }






  // ===== Look for new cards =====
  if ( ! rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Verify if the NUID has been read
  if ( ! rfid.PICC_ReadCardSerial()) {
    Serial.println(F("Debug2"));
    return;
  }

  Serial.println(F("\nNew Card"));

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }


  //Serial.println(F("A new card has been detected."));
  digitalWrite(LEDPIN, LED_ON);
  ledMillis = millis();

  cardId = 0;

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
    cardId = cardId + nuidPICC[i];            //Make the card ID
  }


  //  if (cardId == unlockCard1 || cardId == unlockCard2) {
  // Any card/tag opens the drawer.
  //Serial.println(F("Drawer unlocked"));

  //Make sure the unlock is noticed
  for (int i = 0; i < 5; i++) {
    servo.write(unlockedPosition);
    delay(200);
    servo.write(lockedPosition);
    delay(200);
  }
  drawerUnlock();
  drawerMillis = millis();                //Reset the drawer timer
  //  }


  // show the card ID
  char buf [6];
  sprintf (buf, "%04i", cardId);
  client.publish(msgTopic, buf);
  Serial.println(buf);


  Serial.print(F("cardId= "));
  Serial.print(cardId);
  Serial.println();



  // Dump debug info about the card; PICC_HaltA() is automatically called
  //  rfid.PICC_DumpToSerial(&(rfid.uid));


  // Halt PICC
  rfid.PICC_HaltA();
  delay(10);

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  delay(10);                            // Prevents accidental duplicate reads

}
