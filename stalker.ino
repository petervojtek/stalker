#include <EEPROM.h>
#include <lowpower.h>

// set to true to debug and see history of door open/close logs
// set to false in production mode
#define WITH_SERIAL_OUTPUT true 

// RTC parts of code are based on http://www.instructables.com/id/Setting-the-DS1307-Real-Time-Clock-using-the-Seria/
// which means this source code is licensed under Attribution-NonCommercial-ShareAlike

#include <Wire.h>
#define RTC_DIGITAL_POWER_PIN 9 // we turn off the rtc board between measurements to save battery
const int DS1307 = 0x68; // Address of DS1307 from data sheets

byte second = 0;
byte minute = 0;
byte hour = 0;
byte weekday = 0;
byte monthday = 0;
byte month = 0;
byte year = 14; // fixed, we do not expect to run more than one year from battery

// door status
#define HALL_SENSOR_ANALOG_READ_PIN 0 
#define DOOR_OPEN 0
#define DOOR_CLOSED 1
#define HALL_SENSOR_DIGITAL_POWER_PIN 8 // we turn off the hall sensor between measurements to save battery

boolean doorStatusHasChanged = false;
int previousDoorStatus = DOOR_CLOSED;
int currentDoorStatus = DOOR_CLOSED;

void setup() {
  pinMode(HALL_SENSOR_DIGITAL_POWER_PIN, OUTPUT); 
  digitalWrite(HALL_SENSOR_DIGITAL_POWER_PIN, LOW);

  pinMode(RTC_DIGITAL_POWER_PIN, OUTPUT); 
  digitalWrite(RTC_DIGITAL_POWER_PIN, LOW); 

  Wire.begin();
  if(WITH_SERIAL_OUTPUT){
    Serial.begin(9600);
    delay(2000);
    printHistory();
    delay(10000);
    Serial.println("starting");
  } 
  else {
    delay(10000);
  }
}

void loop() {
  LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF); // check door status every 2 seconds
  doorStatusHasChanged = updateDoorStatus();
  if(doorStatusHasChanged){
    readTime();
    printTime();
    printCurrentDoorStatus();
    storeDoorStatusToEeprom();
  }
}

void printCurrentDoorStatus(){
  if(WITH_SERIAL_OUTPUT){
    if(currentDoorStatus == DOOR_OPEN){
      Serial.println("door status changed to OPEN");
    } 
    else {
      Serial.println("door status changed to CLOSED");
    }
  }
}

long hallValue;

boolean updateDoorStatus(){
  previousDoorStatus = currentDoorStatus;

  digitalWrite(HALL_SENSOR_DIGITAL_POWER_PIN, HIGH);
  LowPower.powerDown(SLEEP_15Ms, ADC_OFF, BOD_OFF);  
  long hallValue = analogRead(HALL_SENSOR_ANALOG_READ_PIN);
  digitalWrite(HALL_SENSOR_DIGITAL_POWER_PIN, LOW);

  if(hallValue > 100){
    currentDoorStatus = DOOR_OPEN;
  } 
  else {
    currentDoorStatus = DOOR_CLOSED;
  }

  return(previousDoorStatus != currentDoorStatus);
}

byte decToBcd(byte val) {
  return ((val/10*16) + (val%10));
}
byte bcdToDec(byte val) {
  return ((val/16*10) + (val%16));
}


// if you want to calibrate current time of rtc board, call this function
// you will need to call this function when you replace the CR2032 battery
void setTime() {
  digitalWrite(RTC_DIGITAL_POWER_PIN, HIGH);
  LowPower.powerDown(SLEEP_15Ms, ADC_OFF, BOD_OFF);  

  year = 14; // 00-99
  month = 10; // 1-12
  monthday = 4; // 1-31
  weekday = 7; // 1 Sun | 2 Mon | 3 Tues | 4 Weds | 5 Thu | 6 Fri | 7 Sat
  hour = 10; // 0-23
  minute = 33; // 0-59
  second = 0;

  Wire.beginTransmission(DS1307);
  Wire.write(byte(0));
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(weekday));
  Wire.write(decToBcd(monthday));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.write(byte(0));
  Wire.endTransmission();

  digitalWrite(RTC_DIGITAL_POWER_PIN, LOW);
}

void printTime() {
  if(WITH_SERIAL_OUTPUT){
    char buffer[3];
    Serial.print(", 20");
    Serial.print(year);
    Serial.print("-");
    Serial.print(month);
    Serial.print("-");
    Serial.print(monthday);
    Serial.print(" ");
    Serial.print(hour);
    Serial.print(":");
    sprintf(buffer, "%02d", minute);
    Serial.println(buffer);
  }
}

void readTime() {
  digitalWrite(RTC_DIGITAL_POWER_PIN, HIGH);
  LowPower.powerDown(SLEEP_15Ms, ADC_OFF, BOD_OFF);  

  Wire.beginTransmission(DS1307);
  Wire.write(byte(0));
  Wire.endTransmission();
  Wire.requestFrom(DS1307, 7);
  second = bcdToDec(Wire.read());
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read());
  weekday = bcdToDec(Wire.read());
  monthday = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year = bcdToDec(Wire.read());

  digitalWrite(RTC_DIGITAL_POWER_PIN, LOW);
}

int recordCounter = 0;
int eepromPointer;

// we use internal atmega328 1kb eeprom
// the approach below is far from storage-efficient - we use 5 bytes to store timestamp+door status, so that we can store 200 door statuses.

// the same can be achieved by using only three bytes (month: 4bits, day: 5bits, hour: 5 bits, minute: 6bits, door status:1bit)
// you may find bitRead and writeBit arduino functions useful for the purpose

void storeDoorStatusToEeprom(){
  eepromPointer = 1 + (recordCounter * 5);


  EEPROM.write(eepromPointer, month); 
  EEPROM.write(eepromPointer + 1, monthday);
  EEPROM.write(eepromPointer + 2, hour);
  EEPROM.write(eepromPointer + 3, minute);
  EEPROM.write(eepromPointer + 4, currentDoorStatus);

  recordCounter++;
  EEPROM.write(0, recordCounter);
}

void printHistory(){
  int historyRecordsCount = EEPROM.read(0);
  for(int i = 0; i < historyRecordsCount; i++){
    eepromPointer = 1 + (i * 5);
    month = EEPROM.read(eepromPointer);
    monthday = EEPROM.read(eepromPointer + 1);
    hour = EEPROM.read(eepromPointer + 2);
    minute = EEPROM.read(eepromPointer + 3);
    currentDoorStatus = EEPROM.read(eepromPointer + 4);
    printTime();
    printCurrentDoorStatus();
  }
}







