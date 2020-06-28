//========== Loop ==========
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
