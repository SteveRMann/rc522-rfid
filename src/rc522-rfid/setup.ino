//========== Setup ==========
void setup() {
  beginSerial();
  EEPROM.begin(40);                      //holds 2 integers and one long int.
  setup_wifi();
  start_OTA();
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
  drawerTime = 30000;

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
