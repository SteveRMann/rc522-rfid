#define SKETCH_NAME "rc522-rfid.ino"
#define SKETCH_VERSION "V1.2"
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
   Version 1.0  Cloned from monkey_detector.ino
                because it really is just an RFID sensor on a nodemcu.
   Version 1.1  Changed default unlock time to 30-seconds, fixed a bug in publishStatus()
   Version 1.2  Added OTA

*/


#define LED_ON HIGH
#define LED_OFF LOW

#define LOCKED HIGH
#define UNLOCKED LOW
bool tuneState = false;
bool lockStatus;
bool tuneFlag = false;


// ===== timers =====
unsigned long drawerMillis;           // Used to time how long the drawer stays unlocked.
unsigned long drawerTimeRemaining;
unsigned long statusTimer;
unsigned long tuneMillis;             // Timer for tuning
unsigned long tuneTime = 1000;        // Tune duration in ms
unsigned long ledMillis;              //LED ON timer
unsigned long ledTime = 300;          //LED ON duration


#include <ArduinoOTA.h>
#include <SPI.h>
#include <MFRC522.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "D:\River Documents\Arduino\libraries\Kaywinnet.h"
#include <EEPROM.h>
#include <Servo.h>
Servo servo;

// setup_wifi vars for OTA and WiFi
char macBuffer[24];       // Holds the last three digits of the MAC, in hex.
char hostNamePrefix[] = HOSTPREFIX;
char hostName[24];        // Holds hostNamePrefix + the last three bytes of the MAC address.

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
