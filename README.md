Stalker
=======

_"The Zone wants to be respected. Otherwise it will punish."_ &mdash;  [ Stalker, 1979](http://en.wikipedia.org/wiki/Stalker_(1979_film))

Arduino&ndash;based door stalking device.

```
2014-10-03 11:30 door status changed to OPEN 
2014-10-03 11:32 door status changed to CLOSED
```

![Stalker](https://raw.githubusercontent.com/petervojtek/stalker/master/images/00.jpg)

Features
--------

* Battery powered: will run for 3 months from `600mAh` lion battery.
* Cheap (20 USD): so that you can attach it to public-space door
* Hall sensor (magnet) based door status detection 
* Log entries are persistently stored to Arduino's EEPROM

Wiring & Code
-------------

![Stalker fritzing diagram](https://raw.githubusercontent.com/petervojtek/stalker/master/images/wiring.png)

Few notes on the [arduino source code](https://github.com/petervojtek/stalker/blob/master/stalker.ino):
* The circuit and source code is optimized to maximize battery life.
* Bare arduino is powered directly from lion battery without any voltage regulation. 
* Hall sensor is powered from arduino's digital pin (and not directly from battery) to cut its power consumption as much as possible. 
 * Similarly, real time clock is powered from arduino's digital pin for the same purpose.
* [LowPower.h](https://github.com/rocketscream/Low-Power) sleep mode is used as much as possible.
* Records are stored into Arduino's internal 1kB EEPROM.
* Use `printHistory()` function to get door logs from EEPROM.

Assembly
--------

You have to attach the Stalker and magnet on a door so that:
* the magnet is near the hall sensor when door is closed
* distance between magnet and hall sensor increase as door opens

The [KY-003 Hall sensor](http://www.ebay.com/itm/KY-003-Hall-Magnetic-Sensor-Module-for-Arduino-AVR-PIC-Good-/370886143225) has a LED which blinks when magnet is near, which is useful for tuning the hall sensor-to-magnet position.

Battery Life
------------

* If you want to achieve good battery life (months), you cannot use regular arduino which is too greedy in terms of current consumption. Instead [use bare atmega328 chip with arduino bootloader](https://github.com/petervojtek/diy/wiki/Arduino-with-Very-Low-Power-Consumption).
* Current draw is `0.20mA` when Stalker is on low-power sleep. 
* Every 2 seconds there is higher power demand as the hall sensor wakes up for 15 milliseconds and measures door status.
 * Only when hall sensor detects door status change, real time clock circuit is powered on and current time is fetched.
* I assume the average current draw is not more than `0.25mA` (unfortunately it is not possible to measure the current draw peaks properly with simple multimeter).
* `600mAh battery / 0.25 mA = 2400 hours = 100 days`

Bill of Material
----------------

Prices below include shipping to EU.

* [Atmega328 bare Arduino Uno Kit](http://www.ebay.de/itm/ATMEGA328-PU-ARDUINO-UNO-KIT-mit-5V-Spannungsstabilisator-MCU-A449-/261357577770?ssPageName=ADME:L:OC:US:3160): 7.3 Euro
 * [mini breadboard](http://www.ebay.com/itm/Mini-Nickel-Plating-Breadboard-170-Tie-points-for-Arduino-Shield-Black-TN2F-/281265100067): 0.7 Euro
 * few wires 
* [KY-003 Hall sensor](http://www.ebay.com/itm/KY-003-Hall-Magnetic-Sensor-Module-for-Arduino-AVR-PIC-Good-/370886143225): 1.74 Euro
* [I2C Real Time Clock](http://www.ebay.com/itm/Arduino-I2C-RTC-DS1307-AT24C32-Real-Time-Clock-Module-For-AVR-ARM-PIC-SMD-/170910326110): 0.88 Euro
 * [CR2302 battery](http://www.ebay.com/itm/5-x-CR2032-DL2032-ECR2032-5004LC-3-Volt-Button-Cell-Battery-SL-/291197049625) ~ 0.2 Euro
* [600mAh lion battery](http://www.ebay.ca/itm/3-7V-600mAh-Lipo-Battery-for-Walkera-Syma-WLtoys-V929-V949-V959-V212-V222-/380960159510): 4.8 Euro
* Magnet
* Enclosure

Sum: 15.6 Euro (~ 19.52 USD)

TODOs
-----

* The current data format used to log door record into EEPROM is not effective - one log entry occupies 5 bytes, can be optimized to 3 bytes.
* The internal arduino's EEPROM is 1kB, which corresponds to 200 log entries - which is not enough for a longer period of stalking. Thankfully, The [RTC board DS1307 contains 32kB EEPROM](http://www.instructables.com/id/Setting-the-DS1307-Real-Time-Clock-using-the-Seria/), which you can use instead.
 * I do not recommend you to use SD card for logging - it will be more expensive (you will need an SD card and board for writing SD cards) and personally I was unable to achieve low power consumption with the SD card board. 

If you would like to contribute to Stalker's source code, you are welcome to share your enhancements via pull request.



