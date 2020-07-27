


//============================= printHex() =============================
/**
   Routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}


// ============================= printDec() =============================
/**
   Routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}


// ============================= eeprom() =============================
/**
  Here is a code for writing one int val at some position pos in the EEPROM:
*/

void eeWriteInt(int pos, int val) {
  byte* p = (byte*) &val;
  EEPROM.write(pos, *p);
  EEPROM.write(pos + 1, *(p + 1));
  EEPROM.write(pos + 2, *(p + 2));
  EEPROM.write(pos + 3, *(p + 3));
  EEPROM.commit();
}

// And , of course, you need to read it back:

int eeReadInt(int pos) {
  int val;
  byte* p = (byte*) &val;
  *p        = EEPROM.read(pos);
  *(p + 1)  = EEPROM.read(pos + 1);
  *(p + 2)  = EEPROM.read(pos + 2);
  *(p + 3)  = EEPROM.read(pos + 3);
  return val;
}

void eeWriteLong(int pos, long val) {
  byte* p = (byte*) &val;
  for (int i = 0; i < 32; i++) {
    EEPROM.write(pos + i, *(p + i));
  }
  EEPROM.commit();
}

long eeReadLong(int pos) {
  long val;
  byte* p = (byte*) &val;
  for (int i = 0; i < 32; i++) {
    *(p + i)  = EEPROM.read(pos + i);
  }
  return val;
}


void writeEprom() {
  //Fill the servo struct then write it to the EEPROM
  myServo.lockedPosition = lockedPosition;
  myServo.unlockedPosition = unlockedPosition;
  myServo.drawerTime = drawerTime;
  EEPROM_writeAnything(0, myServo);

  if (EEPROM.commit()) {
    Serial.println(F("EEPROM values written"));
  } else {
    Serial.println(F("ERROR! EEPROM commit failed"));
  }

  //Read it back
  EEPROM_readAnything(0, myServo);
  lockedPosition = myServo.lockedPosition;
  unlockedPosition = myServo.unlockedPosition;
  drawerTime = myServo.drawerTime;
  showStatus();
}


// ============================= drawerLock() drawerUnlock() =============================
void drawerLock() {
  if (lockStatus == UNLOCKED) {
    //Serial.println(F("Drawer Locked"));
    servo.write(lockedPosition);
    lockStatus = LOCKED;
    publishStatus();
  }
}

void drawerUnlock() {
  if (lockStatus == LOCKED) {
    //Serial.println(F("Drawer Unlocked"));
    servo.write(unlockedPosition);
    drawerMillis = millis();                //Reset the drawer timer
    lockStatus = UNLOCKED;
    publishStatus();
  }
}


// ============================= publishStatus() =============================
void publishStatus() {
  showStatus();
  char buf [20];
  sprintf (buf, "%d,%d,%u,%d", lockedPosition, unlockedPosition, drawerTime, lockStatus);
  client.publish(statusTopic, buf);
}


// ============================= showStatus() =============================
void showStatus() {
  Serial.println(F("--STATUS--"));
  Serial.println(F("lock, unlock, time, status"));
  Serial.print(lockedPosition);
  Serial.print(F(", "));
  Serial.print(unlockedPosition);
  Serial.print(F(", "));
  Serial.print(drawerTime);
  Serial.print(F(", "));
  if (lockStatus == LOCKED) {
    Serial.print(F("Locked"));
  } else {
    Serial.print(F("Unlocked"));
  }
  Serial.print(F("("));
  Serial.print(lockStatus);
  Serial.println(F(")"));
}


// ============================= ltrim() =============================
//function to remove the first character from a c-string.
// Usage:
//   Remove leading zeros
//   if (myHour[0] == 48) ltrim(myHour);

char *ltrim(char *contents) {                    // Hard to get your head around it,
  char *p = contents;                            // but p simply points to the first character of the argument.
  unsigned int i;
  for (i = 0; i < strlen(contents); i++)
  {
    p[i] = contents[i + 1];
  }
  return p;
}
