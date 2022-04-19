# M5Stack-NTP-Clock

This sketch shows you the current time from the nearest NTP server on the builtin M5Stack display.

It has the ability to show am and pm, as well as do military time or non military time.

You need this library tho so get installing: https://github.com/taranais/NTPClient

Notes about modifications/alterations in this fork by @PaulskPt:

Modifications in the Arduino library ```NTPClient```:

To ```NTPClient.cpp``` I added a function ```tz_nato()```.
This function is added to undo the limitation explaned in the file NTPClient.cpp (from author: ```joeybab3```)
```currently assumes UTC timezone, instead of using this->_timeOffset```.
The function tz_nato() is called just before the end in function getFormattedDate().
Parameter: a time-zone in the range "-12" ... "+12". This function returns a ```NATO timze-zone letter``` in the range "A" ... "Y"  + "Z" (except "J").

In NTPClient.cpp I modified the functions: ```getFormattedTime()``` and ```getFormattedDate()```.
The function getFormattedDate can now be called with a 2nd parameter ```disp_gmt```. This parameter defaults to ```true```.
Example: getFormattedDate(0, false);
In that case the function will return the formattedDate with as last character the NATO time-zone letter. 
Example: "2022-04-19T00:01:46A" (for the time-zone UTC + 1 hour) (= UTC + 3600 seconds)

whereas in the calling .ino sketch the NTPClient was defined as follows:
```
#define NTP_OFFSET  +3600              // for Europe/Lisbon       was: -28798 // In seconds for los angeles/san francisco time zone
#define NTP_INTERVAL 60 * 1000         // In miliseconds
#define NTP_ADDRESS  "pt.pool.ntp.org" // was: "pool.ntp.org"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
```

Modifications in this fork:

```
a) moved date/time handling from ```loop()``` to dt_handler();
b) added functionality to retrieve WiFi credentials and DEBUG_FLAG from CD card, file ```secrets.h```
c) set/clear global flag ```my_debug``` from retrieved DEBUG_FLAG
d) added functionality for button presses of BtnA, BtnB and BtnC. In this moment only BtnA and BtnC are in use.
e) added function to update date/time of built-in RTC from NTP server on internet. 
f) added functionality to use BtnA to force re-synchronization of the built-in RTC from a NTP server.
g) in loop() added a elapsed time calculation.
h) in loop() added a call to dt_handler() every 5 minutes with flag lRefresh true to force re-synchronization of the built-in RTC from a NTP server.
i) in dt_handler() added functionality for a 24 hour clock instead of 12 hour with am/pm
j) in dt_handler() added functionality to update only the most frequently changed data: the time. The date and day-of-the-week will only updated when necessary.
```