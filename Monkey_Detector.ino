#define SKETCH_NAME "Monkey_Detector.ino"
#define SKETCH_VERSION "V1.1"
#define HOSTPREFIX "Monkey"


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
*/


// setup_wifi vars
char macBuffer[24];       // Holds the last three digits of the MAC, in hex.
char hostNamePrefix[] = HOSTPREFIX;
char hostName[24];        // Holds hostNamePrefix + the last three bytes of the MAC address.

#define LED_ON HIGH
#define LED_OFF LOW

//Servo positions
#define LOCKED 0
#define UNLOCKED 90

// Drawer timer
unsigned long drawerMillis;           // Used to time how long the drawer stays unlocked.
unsigned long drawerTime = 3000;     // How long the drawer should be unlocked.

//LED Blink timer
unsigned long ledMillis;              //LED ON timer
unsigned long ledTime = 300;          //LED ON duration

#include <SPI.h>
#include <MFRC522.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "D:\River Documents\Arduino\libraries\Kaywinnet.h"


#include <Servo.h>
Servo servo;



// Initialize the espClient. The espClient name must be unique
WiFiClient Monkey;
PubSubClient client(Monkey);



#ifndef mqtt_server
#define mqtt_server "192.168.1.124"
#endif
#define client_name "Monkey"           //Must be unique

char msgIN;

#define NODENAME "Monkey"
const char *statusTopic = NODENAME "/status";
const char *connectName =  NODENAME "xyzzMonkey";             //Must be unique
const char* cmdTopic = NODENAME "/led";
const char* msgTopic = NODENAME "/msg";

//const int unlockCard = 366;       // This is the card number we're looking for
const int unlockCard = 627;         // This card number unlocks the drawer
const int lockCard = 620;           // This card number locks the drawer

constexpr uint8_t RST_PIN = 5;      // Same as const byte RST_PIN
constexpr uint8_t SS_PIN = 4;
const int LEDPIN = 10;              // NodeMCU SD3 (GPIO10). Blink this LED when a new card is detected.

// Buttons
const int unlockSwitchPin = D8;   //Switches 3V3 to D8, D8 pulled down.
int unlockSwitchValue;

MFRC522 rfid(SS_PIN, RST_PIN);      // Create an instance of the class
MFRC522::MIFARE_Key key;


// Init the array that will store new Tag ID (nuid)
byte nuidPICC[4];
int cardId = 0;                     // Card ID (Sum of NUID byes)










//============================= Setup =============================
void setup() {
  beginSerial();

  Serial.print(F("msgTopic= "));
  Serial.println(msgTopic);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  SPI.begin();                    // Init SPI bus
  rfid.PCD_Init();                // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("Scan the MIFARE Classsic NUID."));
  //Serial.print(F("Using the key:"));
  //printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();


  pinMode(LEDPIN, OUTPUT);
  pinMode(unlockSwitchPin, INPUT);  //You can't use input_pullup on pins 15 (D8) and 16 (D0).



  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.println();
  Serial.print(F("MAC Address: "));
  Serial.println(macToStr(mac));
  Serial.println();


  //Start with the servo in the locked position
  servo.attach(2); //D4
  servo.write(UNLOCKED);
  delay(500);
  servo.write(LOCKED);
  delay(200);
}




//============================= Loop =============================
void loop() {


  // Read the Unlock Switch value (D8)
  unlockSwitchValue = digitalRead(unlockSwitchPin);
  if (unlockSwitchValue == HIGH) {
    //digitalWrite(LedPin, LED_ON);                //High=LED ON.
    servo.write(UNLOCKED);
    drawerMillis = millis();                       //Start a timer to lock it again
    //delay(500);
  }

  if (millis() - drawerMillis > drawerTime) {     //If the drawer has been unlocked for a while, lock it.
    //Serial.println(F("Locked"));
    //digitalWrite(LedPin, LED_OFF);
    servo.write(LOCKED);
    //delay(100);
  }



  if (millis() - ledMillis > ledTime) {          //If the LED has been on for a while, turn it off.
    digitalWrite(LEDPIN, LED_OFF);
  }




  if (!client.connected()) {
    reconnect();
  }
  delay(100);             // Adds stability to MQTT connection.
  client.loop();

  if (millis() - drawerMillis > drawerTime) {     //If the drawer has been unlocked for a while, lock it.
    //Serial.println(F("Locked"));
    servo.write(LOCKED);
    //delay(100);
  }



  // Look for new cards
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been read
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  //Serial.print(F("PICC type: "));
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
  ledMillis=millis();

  cardId = 0;

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
    cardId = cardId + nuidPICC[i];            //Make the card ID
  }


  if (cardId == unlockCard) {
    // Found the card we're looking for.
    // Unlock the drawer
    Serial.println(F("Drawer unlocked"));
    servo.write(UNLOCKED);

    //Start a 30-second timer to lock it again
    drawerMillis = millis();
  }


  if (cardId == lockCard) {
    // Lock the drawer
    Serial.println(F("Drawer locked"));
    servo.write(LOCKED);
  }



  char buf [6];
  sprintf (buf, "%04i", cardId);
  client.publish(msgTopic, buf);
  Serial.println(buf);


  Serial.print(F("cardId= "));
  Serial.print(cardId);
  Serial.println();



  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  //delay(450);                            // Prevents accidental duplicate reads

}







// ============================= mqtt callback =============================
// This function is executed when some device publishes a message to a topic that this ESP8266 is subscribed to.
void callback(String topic, byte * message, unsigned int length) {

  Serial.println();
  Serial.println();
  Serial.print(F("Message arrived on topic: "));
  Serial.print(topic);
  Serial.println(F(""));

  Serial.print(F("messageString: '"));

  // Convert the character array to a string
  String messageString;
  for (int i = 0; i < int(length); i++) {
    messageString += (char)message[i];
  }
  messageString.trim();
  messageString.toUpperCase();          //Make the string upper-case


  Serial.print(messageString);
  Serial.println(F("'"));
  Serial.print(F("Length= "));
  Serial.print(length);
  Serial.println();

  if (topic == cmdTopic) {
    Serial.println(F("topic == cmdTopic"));
    if (messageString == "ON") {
      Serial.println(F("messageString == 'ON'"));
      Serial.println(F("Lights ON"));
      digitalWrite(LEDPIN, HIGH);
    } else {
      if (messageString == "OFF") {
        Serial.println(F("messageString == 'OFF'"));
        Serial.println(F("Lights OFF"));
        digitalWrite(LEDPIN, LOW);
      }
    }
  }
}           //callback
