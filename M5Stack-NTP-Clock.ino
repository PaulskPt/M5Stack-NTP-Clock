/*
 * Forked from: https://github.com/joeybab3/M5Stack-NTP-Clock by @PaulskPt
 * The README.md says: You need this library tho so get installing: https://github.com/taranais/NTPClient
 * I already had an NTPClient library installed. I had to delete that one because it did not have the function: getFormattedDate()
 * Modifications in this fork:
 * a) use of <M5Core2.h> instead of <M5Stack.h>
 * b) moved date/time handling from loop() to dt_handler();
 * c) added functionality to retrieve WiFi credentials and DEBUG_FLAG from CD card, file: '/secrets.h'
 * d) set/clear global flag my_debug from retrieved DEBUG_FLAG
 * e) added functionality for button presses of BtnA, BtnB and BtnC. In this moment only BtnA and BtnC are in use.
 * f) added function to update date/time of built-in RTC from NTP server on internet. 
 * g) added functionality to use BtnA to force re-synchronization of the built-in RTC from a NTP server.
 * h) in loop() added a elapsed time calculation.
 * i) in loop() added a call to dt_handler() every 5 minutes with flag lRefresh true to force re-synchronization of the built-in RTC from a NTP server.
 * j) in dt_handler() added functionality for a 24 hour clock instead of 12 hour with am/pm
 * k) in dt_handler() added functionality to display the day-of-the-week (using the added global char array daysOfTheWeek[7][12])
 * l) added function disp_msg() to display informative/warning or error messages
 * m) added the blinking of the power led to indicate when date and/or time is read and displayed
 * n) added use of M5Stack Axp module to control built-in Power LED, display backlight and eventually the possibility to switch off the device.
 * o) added a function disp_frame() which builds the static texts on the display
 * p) in dt_handler() added functionality to update only the most frequently changed data: the time. The date and day-of-the-week will only updated when necessary.
 * The measures in o) and p) make the appearance of the display more 'quiet'.
*/
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <M5Core2.h>

boolean my_debug = NULL;

FILE myFile;

#ifndef USE_PSK_SECRETS
#define USE_PSK_SECRETS (1)   // Comment-out when defining the credentials inside this sketch
#endif

#ifdef USE_PSK_SECRETS
#include "secrets.h"
#endif

#ifdef USE_PSK_SECRETS
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[]           = SECRET_SSID;     // Network SSID (name)
char password[]       = SECRET_PASS;     // Network password (use for WPA, or use as key for WEP)
char debug_flag[]     = DEBUG_FLAG;      // General debug print flag
char ntp_local_flag[] = NTP_LOCAL_FLAG;  // Defines to use local NTP server or world pool server
char ntp_local_url[]  = NTP_LOCAL_URL;    // URL of local NTP server pool
#else
char ssid[]           = "SSID";         // your network SSID (name)
char password[]       = "password"; // your network password
char debug_flag[]     = "0";
char ntp_local_flag[] = "0";
char ntp_local_url[]  = "pt.pool.ntp.org";
#endif

boolean ntp_local = false; // default use worldwide ntp pool
char ntp_url[]= "";
//char* ntp_url = "";
/*
 * See: C:\Users\<User>\Documenten\Arduino\arduino-esp32-master\tools\sdk\esp32\include\driver\include\driver\ledc.h
 * @brief LEDC channel configuration
 *        Configure LEDC channel with the given channel/output gpio_num/interrupt/source timer/frequency(Hz)/LEDC duty resolution
 *
 * @param ledc_conf Pointer of LEDC channel configure struct
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 */
//esp_err_t ledc_channel_config(const ledc_channel_config_t* ledc_conf);

#define LEDC AXP_IO1

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String fDate;
String dayStamp;
String timeStamp;
boolean btnA_state = false;
boolean btnB_state = false;
boolean btnC_state = false;
boolean lRefresh = false;

#define NTP_OFFSET  +3600              // for Europe/Lisbon       was: -28798 // In seconds for los angeles/san francisco time zone
#define NTP_INTERVAL 60 * 1000         // In miliseconds
#define NTP_ADDRESS  "pool.ntp.org"

char* ntp_global_url = "pool.ntp.org";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

#define TFT_GREY 0x5AEB
boolean lStart = true;
boolean lStop = false;
String TITLE =  "NTP Synchronized Clock";
unsigned int colour = 0;
int hori[] = {0, 80, 160, 240};
int vert[] = {40, 80, 120, 160, 200};
unsigned long t_start = millis();

RTC_DateTypeDef DateStruct;
RTC_TimeTypeDef TimeStruct;
String date_old = "";

boolean vars_fm_sd = true;
// ============================ SETUP =================================================
void setup(void) 
{
  boolean SD_present = false;
  Serial.begin(115200);
  Serial.println();
  Serial.println(TITLE);

  #define TF_card_CS_pin 4
  
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(TF_card_CS_pin))
  {
    Serial.println(F("initialization failed")); 
    // Use WiFi-credentials linked-in at build process
    // while(true) 
    // { 
    //   delay(3000); // infinite loop
    //}
  }
  else
  {
    SD_present = true;
    Serial.println(F("initialization done."));
  }
  if (SD_present && vars_fm_sd)
  {
     // Read WiFi credentials from SD Card
     if (rdSecrets(SD,"/secrets.h"))
     {
       Serial.print(F("All needed values successfully read from file \""));
       Serial.print("/secrets.h");
       Serial.println("\"");
     }
     else
     {
       Serial.print("None or some values read from file \"");
       Serial.println("/secrets.h");
     }
     Serial.print("Value of ntp_local flag = ");
     Serial.println(ntp_local == 1 ? "true" : "false");
     Serial.print("Value of my_debug flag = ");
     Serial.println(my_debug == 1 ? "true" : "false");
  }

  
  if (ntp_local)
  {
    int le = sizeof(ntp_local_url)/sizeof(ntp_local_url[0]);
    for (uint8_t i = 0; i < le; i++)
    {
      ntp_url[i] = ntp_local_url[i];
    }
  }
  else
  {
    int le = sizeof(ntp_global_url)/sizeof(ntp_global_url[0]);
    for (uint8_t i = 0; i < le; i++)
    {
      ntp_url[i] = ntp_global_url[i];
    }
  }
  
  Serial.print("for NTPClient we will use url: ");
  Serial.println(ntp_url);

  timeClient.chg_url(ntp_url);  // Adjust the NTP pool URL
  
  if (!my_debug)
  {
    Serial.print("Using NTP Pool server: \'");
    Serial.print(ntp_url[0]);
    Serial.println("\'");
    Serial.print("NTP timezone offset from GMT: ");
    if (NTP_OFFSET > 0)
      Serial.print("+");
    Serial.print(NTP_OFFSET);
    Serial.println(" seconds\n");
  }
  //----------------------------------------------
  M5.begin();
  M5.Rtc.begin();  // Initialize the RTC clock

  M5.Axp.SetLDO2(true); 
  M5.Axp.SetDCDC3(true);  // Turn LCD backlight on
  M5.Axp.SetLed(true);
  
  M5.Lcd.clear();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(30, vert[0]-10);
  M5.Lcd.print(TITLE);
  //----------------------------------------------
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  delay(3000);
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  timeClient.begin();
  // ========== TEST ===============
  if (my_debug)
  {
    Serial.println("NATO time-zone\'s test");
    String ofs, tz = "";
    for (int i = 0; i < 4; i++)
    {
      if (i == 0)
        ofs = "-10";
      else if (i == 1)
        ofs = "0";
      else if (i == 2)
        ofs = "+12";
      else if (i == 3)
        ofs = "-14"; // outside range, shout result in "?"
      tz = timeClient.tz_nato(ofs);
      Serial.print("offset = ");
      Serial.print(ofs);
      Serial.print(", NATO time-zone = ");
      Serial.println(tz);
    }
    Serial.println("End-of-test");
  }
  // ======= END-OF-TEST ===========
  disp_frame();
}

// ====================================== END OF SETUP =================================================


boolean rdSecrets(fs::FS &fs, const char * path)
{
  String s = "Reading file \""+(String)path+"\"";
  String s2 = "";
  String sSrch = "";
  String line = "";
  int le = 0;
  int idx = -1;
  int n = -1;
  int fnd = 0;
  boolean lFnd, txt_shown = false;

  const char item_lst[][15] = {"SECRET_SSID", "SECRET_PASS", "DEBUG_FLAG", "NTP_LOCAL_FLAG", "NTP_LOCAL_URL"};
  le = sizeof(item_lst)/sizeof(item_lst[0]);
  
  Serial.printf("%s\n", s);
  File file = fs.open(path);
  if (!file) {
    s = "Failed to open "+String(path)+" for reading";
    Serial.println(s);
    return false;
  }

  while (file.available())
  {
    try 
    {
      Serial.print("rdSecrets(): nr of items to retrieve: ");
      Serial.println(le);
      for (uint8_t i = 0; i < le; i++)
      {
        sSrch = item_lst[i];
        if (file.available())
        { 
          txt_shown = false;
          while (true)
          {
            line = file.readStringUntil('\n'); // was: ('\r\n');
            if (!txt_shown)  // shown text only once
            {
              txt_shown = true;
              Serial.print("searching string: ");
              Serial.println(sSrch);
            }
            idx = line.indexOf(sSrch);
            if (idx> 0)
            {
              fnd++;
              s2 = extract_item(line, idx);
              if (s2.length() > 0)
              {
                if (i == 0)
                {
                  s2.toCharArray(ssid,s2.length()-1);
                  Serial.print("extracted ssid: ");
                  Serial.println(ssid);
                }
                else if (i == 1)
                {
                  s2.toCharArray(password,s2.length()-1);
                  Serial.print("extracted password ");
                  Serial.println(password);
                }
                else if (i == 2)
                {
                  s2.toCharArray(debug_flag,s2.length()-1);
                  Serial.print("extracted debug_flag: ");
                  Serial.println(debug_flag[0]);
                  if (debug_flag[0])
                    my_debug = true;
                  else
                    my_debug = false;
                }
                else if (i == 3)
                {
                  s2.toCharArray(ntp_local_flag,s2.length()-1);
                  Serial.print("extracted ntp_local_flag: ");
                  Serial.println(ntp_local_flag[0]);
                  if (ntp_local_flag[0])
                    ntp_local = true;
                  else
                    ntp_local = false;
                }
                else if (i == 4)
                {
                  s2.toCharArray(ntp_local_url,s2.length()-1);
                  Serial.print("extracted ntp_local_url: ");
                  Serial.println(ntp_local_url);
                }  
                break; // break out of while loop
              }
            }
          }
        }
        else
        {
          throw 241;  // indicate linenr where file.available revealed a false
        }
      }
    }
    catch(int e)
    {
      Serial.print("End of file reached in line: ");
      Serial.println(e);
    }
  }
  file.close();

  if (fnd == le)
    return true;  // Indicate all items were found and handled
  return false;
}

String extract_item(String itm, int idx)
{
  String sRet = "";
  int le = itm.length();
  int n = 0;

  n = itm.indexOf('\"');  // search of a Double quote mark
  if (n == 0)
    n = itm.indexOf('\''); // search for a Single quote mark

  if (n > 0)
    sRet = itm.substring(n+1, le);

  return sRet;
}

void disp_frame()
{
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(30, vert[0]-10);
    M5.Lcd.print(TITLE);
    M5.Lcd.setCursor(0, vert[1]);
    M5.Lcd.print("Day: ");
    M5.Lcd.setCursor(0, vert[2]);
    M5.Lcd.print("Date: ");
    M5.Lcd.setCursor(0, vert[3]);
    M5.Lcd.print("Time: ");
}

void disp_msg(String msg, int linenr)
{
  if (linenr == 1)
  {
      M5.Lcd.clear(); // Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(30, vert[2]-10);
      M5.Lcd.print(msg);
  }
  if (linenr == 2)
  {
    M5.Lcd.setCursor(30, vert[3]-10);
    M5.Lcd.print(msg);
  }
  Serial.println(msg);
  delay(2000);
}


void dt_handler(boolean lRefr)
{
    int hour, minute, second = 0;
    int year, month, day, weekDay = 0;
    int height_date = vert[3]-vert[1];
    int height_time = vert[4]-vert[3];
    String TAG = "dt_handler(): ";
    String dateStamp, dayStamp, monthStamp, yearStamp = "";
    String wd, yearStampSmall, timeStamp, hourStamp, minuteStamp, secondStamp = "";
    String hrs,mins,secs = "";
    String tz_ltr = "?";
    boolean use_local_time = ntp_local; // This is temporary. There should be a separate variable in secrets.h to indicate use of local time or GMT

    if (lRefr)  // Refresh (from NTP Server)
    {
      timeClient.update();
      fDate = timeClient.getFormattedDate(0, use_local_time);
      Serial.print("DateTime sync from NTP server: ");
      Serial.println(fDate);
      Serial.println();

      int splitT = fDate.indexOf("T");
      
      dateStamp      = fDate.substring(0, splitT);
      yearStamp      = fDate.substring(0, 4);
      yearStampSmall = fDate.substring(2, 4);
      monthStamp     = fDate.substring(splitT-5, splitT-3);      
      dayStamp       = fDate.substring(splitT-2, splitT);

      timeStamp      = fDate.substring(splitT+1, fDate.length()-1);
      hourStamp      = timeStamp.substring(0,2);
      minuteStamp    = timeStamp.substring(3,5);
      secondStamp    = timeStamp.substring(6,8);
      weekDay        = timeClient.getDay();
      wd             = daysOfTheWeek[weekDay];
      tz_ltr         = timeClient.tz_nato(String(NTP_OFFSET/3600));

      if (!my_debug)
      {
        Serial.print(TAG);
        Serial.print("timeClient.getFormattedDate() = \"");
        Serial.print(fDate);
        Serial.println("\"\n");
      }
      
      year   = yearStamp.toInt();
      month  = monthStamp.toInt();
      day    = dayStamp.toInt();
      hour   = hourStamp.toInt();
      minute = minuteStamp.toInt();
      second = secondStamp.toInt();
    
      DateStruct.WeekDay = weekDay;
      DateStruct.Month   = month;
      DateStruct.Date    = day;
      DateStruct.Year    = year;
      M5.Rtc.SetDate(&DateStruct);  // Set the internal RTC with date of NTP
      TimeStruct.Hours = hour;
      TimeStruct.Minutes = minute;
      TimeStruct.Seconds = second;
      M5.Rtc.SetTime(&TimeStruct);  // Set the internal RTC with time of NTP
    }
    else
    {
      M5.Rtc.GetDate(&DateStruct);  // Get the date of the built-in RTC
      year    = DateStruct.Year;
      month   = DateStruct.Month;
      day     = DateStruct.Date;
      weekDay = DateStruct.WeekDay;
      wd      = daysOfTheWeek[weekDay];
      tz_ltr  = timeClient.tz_nato(String(NTP_OFFSET/3600));
      M5.Rtc.GetTime(&TimeStruct);  // Get the time of the built-in RTC
      
      hour    = TimeStruct.Hours;
      minute  = TimeStruct.Minutes;
      second  = TimeStruct.Seconds;
  
      dayStamp       = day < 10 ? "0" + String(day) : String(day);
      monthStamp     = month < 10 ? "0" + String(month) : String(month);
      yearStamp      = String(year);
      dateStamp      = String(year)+"-"+monthStamp+"-"+dayStamp;
      yearStampSmall = yearStamp.substring(2, 4);
      hourStamp      = hour   < 10 ? "0" + String(hour)   : String(hour);
      minuteStamp    = minute < 10 ? "0" + String(minute) : String(minute);
      secondStamp    = second < 10 ? "0" + String(second) : String(second);
    }
  
    boolean isPm = false;
    boolean use_12hr = false;
    int hourNoMilitary;
  
    if (use_12hr)
    {
      if(hour > 12)
      {
        hourNoMilitary = hour - 12;
        isPm = true;
      }
      else
      {
        isPm = false;
        hourNoMilitary = hour;
      }
    }
    else
    {
      hourNoMilitary = hour;
    }
    
    if(hour == 0)
    {
      hour = 0;
      hourNoMilitary = 0; // was: 12;
    }
    String hourStampNoMilitary;
    
    if(hourNoMilitary < 10)
    {
      hourStampNoMilitary = "0"+(String)hourNoMilitary;
    }
    else
    {
      hourStampNoMilitary = (String)hourNoMilitary;
    }
  
    int daysLeft = 0;
    int monthsLeft = 0;
  
    String timeStampNoMilitary = hourStampNoMilitary + ":" + minuteStamp + ":" + secondStamp;    // hh:mm:ss
    String dateStampConstructed = yearStampSmall + "-" + monthStamp + "-" + dayStamp;            // yy-mo-dd

    if (my_debug)
    {
      Serial.print("dateStampConstructed: ");
      Serial.println(dateStampConstructed);
      Serial.print("timeStampNoMilitary:  ");
      Serial.print(timeStampNoMilitary);
      Serial.println("\n");
    }
    M5.Lcd.setTextSize(3);
    //M5.Lcd.fillScreen(BLACK);
    M5.Axp.SetLed(false);
    
    if (use_12hr)
    {
      if (date_old != dateStampConstructed)
      {
        date_old = dateStampConstructed;
        M5.Lcd.fillRect(hori[2], vert[1], M5.Lcd.width()-hori[2], height_date, BLACK);  // wipe out the variable date text 
        M5.Lcd.setCursor(hori[2], vert[1]);
        M5.Lcd.println(wd);
        M5.Lcd.setCursor(hori[2], vert[2]);
        M5.Lcd.println(dateStampConstructed);
      }
      M5.Lcd.fillRect(hori[2], vert[3], M5.Lcd.width()-hori[2], height_time, BLACK); // wipe out the variable time text
      M5.Lcd.setCursor(hori[2], vert[3]);
      M5.Lcd.println(timeStampNoMilitary);
      M5.Lcd.setCursor(hori[2], vert[4]);
      M5.Lcd.println(tz_ltr);  
      
      M5.Lcd.setTextSize(2);
      if(isPm)
      {
        M5.Lcd.setCursor(290, 80);
        M5.Lcd.println("PM");
      }
      else
      {
        M5.Lcd.setCursor(290, 60);
        M5.Lcd.println("AM");
      }
    }
    else  // 24 hr clock
    {
      if (date_old != dateStampConstructed)
      {
        date_old = dateStampConstructed;
        M5.Lcd.fillRect(hori[2], vert[1], M5.Lcd.width()-hori[2], height_date, BLACK);  // wipe out the variable date text
        M5.Lcd.setCursor(hori[2], vert[1]);
        M5.Lcd.println(wd);                  // day of the week
        M5.Lcd.setCursor(hori[2], vert[2]);
        M5.Lcd.print(dateStampConstructed);  // yy-mo-dd
      }
      M5.Lcd.setCursor(hori[2], vert[3]);
      M5.Lcd.fillRect(hori[2], vert[3], M5.Lcd.width()-hori[2], height_time, BLACK); // wipe out the variable time text
      M5.Lcd.setCursor(hori[2], vert[3]);
      M5.Lcd.println(timeStampNoMilitary);
      M5.Lcd.setCursor(hori[2], vert[4]);
      M5.Lcd.println(tz_ltr);
      M5.Lcd.println();   
      M5.Lcd.setTextSize(2);    
    }
    delay(950);
    M5.Axp.SetLed(true);
}

void buttonA_wasPressed()
{
    btnA_state = true;
    disp_msg("Button A was pressed.", 1);
    disp_msg("Refresh RTC from NTP", 2);
    lRefresh = true;
 } 
  
void buttonB_wasPressed()
{
    btnB_state = true;
    disp_msg("Button B was pressed.", 1);
    disp_msg("not implemented yet", 2);
} 
  
void buttonC_wasPressed()
{
    btnC_state = true;
    disp_msg("Button C was pressed.", 1);
    disp_msg("Exit...", 2);
    lStop = true;
}


boolean use_internal_rtc = true;

void loop() 
{
  unsigned long t_current = millis();
  unsigned long t_elapsed = 0;

  while (true)
  {

    M5.update(); // Read the status of the touch and the buttons
  
    if(M5.BtnA.wasPressed())
      buttonA_wasPressed();
    if (M5.BtnB.wasPressed())
      buttonB_wasPressed();
    if (M5.BtnC.wasPressed())
      buttonC_wasPressed();
   
    t_current = millis();
    t_elapsed = t_current - t_start;
  
    if (lStop)
    {
      String s = "Stopped at: "+String(t_elapsed);
      disp_msg(s,1);
      M5.Lcd.fillScreen(BLACK);
      break;
    }
    else
    {
      if (btnA_state || btnB_state || btnC_state)
      {
        if (btnA_state)
          btnA_state = false;
        if (btnB_state)
          btnB_state = false;
        if (btnC_state)
          btnC_state = false;  
          
        M5.Lcd.clear();
        M5.Lcd.setCursor(0,0);
        date_old = ""; // trick to force re-displaying of: a) day-of-the-week; b) yy-mo-dd (the time is alway displayed!)
        disp_frame();
      }

      
      if (t_elapsed % 300000 == 0){  // 30000 milliseconds = 5 minutes
        t_start = millis();
        lRefresh = true;
      }
      
      if (lStart || lRefresh)
      {
        dt_handler(lRefresh);
        lStart = false;
        lRefresh = false;
      }
      else
        dt_handler(false);
    }
  }
  // Infinite loop
  while(true){
    delay(3000);
  }

}
