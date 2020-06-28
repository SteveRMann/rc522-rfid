# RC522-RFID
This is really just a RC522 RFID module on a Node MCU.
It was built to detect an RFID tag on the bottom of a toy monkey.
The sensor was inside a desk. The game, based on escape room games, required the monkey
to be placed right over the sensor to unlock a drawer.

The drawer unlock period is set by publishing a time in ms to topic: monkey/t

This sketch reads RFID cards or tags then publishes the ID code over MQTT
using the topic of "monkey/msg"

The device publishes a status on topic 'monkey/status'every time something changes. 
The format of the status string is:
 lockedPosition, unlockedPosition, drawerTime, lockStatus
Where locked and unlocked position in degrees (servo position), drawerTime in ms,
and lockStatus is HIGH (1)for Locked, and LOW (0) for unlocked.


