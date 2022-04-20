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
The function getFormattedDate can now be called with a 2nd parameter ```use_local_time```. This parameter defaults to ```false```.
Example: getFormattedDate(0, true);
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
a) use of <M5Core2.h> instead of <M5Stack.h>
b) moved date/time handling from loop() to dt_handler();
c) added functionality to retrieve WiFi SECRET_SSID, SECRET_PASSWORD, DEBUG_FLAG, LOCAL_TIME_FLAG, NTP_LOCAL_FLAG, NTP_LOCAL_URL from CD card, file: '/secrets.h'
d) set/clear global flags: my_debug, local_time and ntp_local from retrieved flags
e) added functionality for button presses of BtnA, BtnB and BtnC. In this moment only BtnA and BtnC are in use.
f) added function to update date/time of built-in RTC from NTP server on internet. 
g) added functionality to use BtnA to force re-synchronization of the built-in RTC from a NTP server.
h) in loop() added a elapsed time calculation.
i) in loop() added a call to dt_handler() every 5 minutes with flag lRefresh true to force re-synchronization of the built-in RTC from a NTP server.
j) in dt_handler() added functionality for a 24 hour clock instead of 12 hour with am/pm
k) in dt_handler() added functionality to display the day-of-the-week (using the added global char array daysOfTheWeek[7][12])
l) added function disp_msg() to display informative/warning or error messages
m) added the blinking of the power led to indicate when date and/or time is read and displayed
n) added use of M5Stack Axp module to control built-in Power LED, display backlight and eventually the possibility to switch off the device.
o) added a function disp_frame() which builds the static texts on the display
p) in dt_handler() added functionality to update only the most frequently changed data: the time. The date and day-of-the-week will only updated when necessary.
The measures in o) and p) make the appearance of the display more 'quiet'.
```

This is a 'work-in-progress'.
