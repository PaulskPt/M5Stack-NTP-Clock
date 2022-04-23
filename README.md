# M5Stack-NTP-Clock with sleep

This sketch shows you the current time from the nearest NTP server on the builtin M5Stack display.

It has the ability to show am and pm, as well as do military time or non military time.

You need this library tho so get installing: https://github.com/taranais/NTPClient

```Notes @PaulskPt:```

This branch contains a modified version of my forked version of ```M5Stack-NTP-Clock.ino```. 
For this reason a new filename: ```NTP_Clock_w_sleep.ino```.

This sketch is for the most part the same as the M5Stack-NTP-Clock.ino, except for the following:

a) only BtnA has a function: to re-synchronize the built-in RTC with the datetime stamp of a NTP Server on internet.
BtnB and BtnC have no function.

```Addition:```

b) the whole display is defined as touch area. Now you can touch the display. If it is ON, it will switch OFF.
Also the Power LED (that blinks in the rithm of the seconds updates) will switch off. The clock however
continues to be updated.
If the display is OFF it will switch ON upon touching the display.

