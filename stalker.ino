#include <EEPROM.h>
#include <lowpower.h>

// set to true to debug and see history of door open/close logs
// set to false in production mode
#define WITH_SERIAL_OUTPUT false 

// RTC parts of code are based on http://www.instructables.com/id/Setting-the-DS1307-Real-Time-Clock-using-the-Seria/
// which means this source code is licensed under Attribution-NonCommercial-ShareAlike

#include <Wire.h>
#define RTC_DIGITAL_POWER_PIN 9 // we turn off the rtc board between measurements to save battery
const int DS1307 = 0x68; // i2c Address of DS1307 from data sheets
const int EEPROM_AT24C32 = 0x50; // i2c address of 32kB eeprom, located on the same rtc board

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
  
  // load number of currently stored door records so that we can continue seamlessly
  loadRecordCounter(); 
  
  if(WITH_SERIAL_OUTPUT){
    Serial.begin(9600);
    delay(1000);
    Serial.println("printing history");
    printHistory();
    delay(1000);
    Serial.println("starting");
  } 
  else {
    delay(1000);
  }
  
  // uncomment the next line to clear the eeprom
  //resetRecordCounter();
}

void loop() {
  LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF); // check door status every 2 seconds
  doorStatusHasChanged = updateDoorStatus();
  if(doorStatusHasChanged){
    powerOnRTC();
    
    readTime();
    printTime();
    printCurrentDoorStatus();
    storeDoorStatusToEeprom();
    
    powerOffRTC();
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
  monthday = 27; // 1-31
  weekday = 2; // 1 Sun | 2 Mon | 3 Tues | 4 Weds | 5 Thu | 6 Fri | 7 Sat
  hour = 20; // 0-23
  minute = 22; // 0-59
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
    Serial.print("20");
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
  delay(10);
}


int recordCounter;
int eepromPointer;

void loadRecordCounter(){
  powerOnRTC();
  recordCounter = readEEPROM(0);
  powerOffRTC();
}

void resetRecordCounter(){
  powerOnRTC();
  writeEEPROM(0, 0);
  recordCounter = 0;
  powerOffRTC();
}

// one door record occupies 5 bytes. this can be reduced to 3 bytes if needed, but since we have 32kb eeprom, we have well enough space
void storeDoorStatusToEeprom(){
  eepromPointer = 1 + (recordCounter * 5);

  writeEEPROM(eepromPointer, month); 
  writeEEPROM(eepromPointer + 1, monthday);
  writeEEPROM(eepromPointer + 2, hour);
  writeEEPROM(eepromPointer + 3, minute);
  writeEEPROM(eepromPointer + 4, currentDoorStatus);

  recordCounter++;
  writeEEPROM(0, recordCounter);
}

void powerOnRTC(){
  digitalWrite(RTC_DIGITAL_POWER_PIN, HIGH);
  delay(10);
}

void powerOffRTC(){
  delay(10);
  digitalWrite(RTC_DIGITAL_POWER_PIN, LOW);
}

void printHistory(){
  powerOnRTC();
 
  Serial.println("history count:");
  Serial.println(recordCounter);
  for(int i = 0; i < recordCounter; i++){
    eepromPointer = 1 + (i * 5);
    month = readEEPROM(eepromPointer);
    monthday = readEEPROM(eepromPointer + 1);
    hour = readEEPROM(eepromPointer + 2);
    minute = readEEPROM(eepromPointer + 3);
    currentDoorStatus = readEEPROM(eepromPointer + 4);
    printTime();
    printCurrentDoorStatus();
    
  }
  powerOffRTC();
}


// parts of code below are from http://forum.arduino.cc/index.php?topic=211409.0
void writeEEPROM(unsigned int eeaddress, byte data ) 
{
  Wire.beginTransmission(EEPROM_AT24C32);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
 
  delay(5);
}
 
byte readEEPROM(unsigned int eeaddress ) 
{
  byte rdata = 0xFF;
 
  Wire.beginTransmission(EEPROM_AT24C32);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(EEPROM_AT24C32,1);
 
  if (Wire.available()) rdata = Wire.read();
 
  return rdata;
}

