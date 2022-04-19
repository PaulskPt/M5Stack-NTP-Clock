# M5Stack-NTP-Clock

This sketch shows you the current time from the nearest NTP server on the builtin M5Stack display.

It has the ability to show am and pm, as well as do military time or non military time.

You need this library tho so get installing: https://github.com/taranais/NTPClient

Notes by @PaulskPt:
To NTPClient.cpp I added a function ```tz_nato()```. 
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
