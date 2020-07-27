// ============================= mqtt callback =============================
// Function is called when some device publishes a message to a topic that
// this ESP8266 is subscribed to.
void callback(String topic, byte * message, unsigned int length) {

  int j;
  int k;

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


  if (topic == lockTopic) {
    lockedPosition = messageString.toInt();                   //Set the new servo locked position
    publishStatus();
    writeEprom();

  }

  if (topic == unlockTopic) {
    unlockedPosition = messageString.toInt();                   //Set the new servo locked position
    publishStatus();
    writeEprom();
  }

  if (topic == drawerTimeTopic) {
    drawerTime = messageString.toInt();
    //  myServo.drawerTime = drawerTime;
    publishStatus();
    writeEprom();
  }


  if (topic == cmdTopic) {
    if (messageString.startsWith("STATUS")) {
      publishStatus();
    }

    if (messageString.startsWith("LUT")) {
      // Command format: lut 85,110,900
      // Sets locked position to 85
      // unlocked position to 110,
      // and drawerTime to 900
      // DrawerTime of 0 is 'no change'
      // DrawerTime of 1-9 is 1 to 9 hours
      //
      Serial.println(F("--LUT--"));
      j = messageString.indexOf(',');           //Find the first comma
      k = messageString.indexOf(',', j + 1);    //Find the second comma
      //Parse the values
      String firstValue = messageString.substring(3, j);        // Past the 'LUT'
      String secondValue = messageString.substring(j + 1, k);
      String thirdValue = messageString.substring(k + 1);       // To the end of the string
      //
      lockedPosition = firstValue.toInt();
      if (lockedPosition < 0 || lockedPosition == 0) lockedPosition = myServo.lockedPosition;  //No change
      //
      unlockedPosition = secondValue.toInt();
      if (unlockedPosition < 0 || unlockedPosition == 0) unlockedPosition = myServo.unlockedPosition;  //No change
      //
      drawerTime = thirdValue.toInt();
      if (drawerTime < 0 || drawerTime == 0) drawerTime = myServo.drawerTime;  //No change
      if (drawerTime < 10) drawerTime = drawerTime * 60 * 1000;                //1 to 9 hours
      //

      publishStatus();
      writeEprom();
    }

    if (messageString.startsWith("RESET")) {
      Serial.println(F("Reset"));
      tuneFlag = false;
      drawerUnlock();
      delay(3000);
      drawerLock();
    }

    if (messageString.startsWith("UNLOCK")) {
      Serial.println(F("Unlocking"));
      tuneFlag = false;
      drawerUnlock();
    }

    if (messageString.startsWith("LOCK")) {
      Serial.println(F("Locking"));
      tuneFlag = false;
      drawerLock();
    }

    if (messageString.startsWith("TUNE")) {
      Serial.println(F("Tune"));
      tuneMillis = millis();
      tuneFlag = true;
    }
  }         //cmnd
}           //callback
