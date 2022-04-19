/**
 * The MIT License (MIT)
 * Copyright (c) 2015 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "NTPClient.h"

NTPClient::NTPClient(UDP& udp) {
  this->_udp            = &udp;
}

NTPClient::NTPClient(UDP& udp, int timeOffset) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName) {
  this->_udp            = &udp;
  this->_poolServerName = poolServerName;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName, int timeOffset) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
  this->_poolServerName = poolServerName;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName, int timeOffset, unsigned long updateInterval) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
  this->_poolServerName = poolServerName;
  this->_updateInterval = updateInterval;
}

void NTPClient::begin() {
  this->begin(NTP_DEFAULT_LOCAL_PORT);
}

void NTPClient::begin(int port) {
  this->_port = port;

  this->_udp->begin(this->_port);

  this->_udpSetup = true;
 
}

void NTPClient::chg_url(const char* newPoolServerName)
{
  uint8_t le = sizeof(newPoolServerName)/sizeof(newPoolServerName[0]);
  if (le > 0)
  {
    this->_poolServerName = newPoolServerName;
    Serial.print("this->_poolServerName set to: ");
    Serial.println(this->_poolServerName);
  }
}


bool NTPClient::isValid(byte * ntpPacket)
{
    //Perform a few validity checks on the packet
    if((ntpPacket[0] & 0b11000000) == 0b11000000)        //Check for LI=UNSYNC
        return false;
        
    if((ntpPacket[0] & 0b00111000) >> 3 < 0b100)        //Check for Version >= 4
        return false;
        
    if((ntpPacket[0] & 0b00000111) != 0b100)            //Check for Mode == Server
        return false;
        
    if((ntpPacket[1] < 1) || (ntpPacket[1] > 15))        //Check for valid Stratum
        return false;

    if(    ntpPacket[16] == 0 && ntpPacket[17] == 0 && 
        ntpPacket[18] == 0 && ntpPacket[19] == 0 &&
        ntpPacket[20] == 0 && ntpPacket[21] == 0 &&
        ntpPacket[22] == 0 && ntpPacket[22] == 0)        //Check for ReferenceTimestamp != 0
        return false;

    return true;
}

bool NTPClient::forceUpdate() {
  #ifdef DEBUG_NTPClient
    Serial.println("Update from NTP Server");
  #endif
  // flush any existing packets
  while(this->_udp->parsePacket() != 0)
    this->_udp->flush();
  this->sendNTPPacket();

  // Wait till data is there or timeout...
  byte timeout = 0;
  int cb = 0;
  do {
    delay ( 10 );
    cb = this->_udp->parsePacket();
    
    if(cb > 0)
    {
      this->_udp->read(this->_packetBuffer, NTP_PACKET_SIZE);
      if(!this->isValid(this->_packetBuffer))
        cb = 0;
    }
    
    if (timeout > 100) return false; // timeout after 1000 ms
    timeout++;
  } while (cb == 0);

  this->_lastUpdate = millis() - (10 * (timeout + 1)); // Account for delay in reading the time

  unsigned long highWord = word(this->_packetBuffer[40], this->_packetBuffer[41]);
  unsigned long lowWord = word(this->_packetBuffer[42], this->_packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;

  this->_currentEpoc = secsSince1900 - SEVENZYYEARS;

  return true;
}

bool NTPClient::update() {
  if ((millis() - this->_lastUpdate >= this->_updateInterval)     // Update after _updateInterval
    || this->_lastUpdate == 0) {                                // Update if there was no update yet.
    if (!this->_udpSetup) this->begin();                         // setup the UDP client if needed
    return this->forceUpdate();
  }
  return true;
}

unsigned long NTPClient::getEpochTime() {
  return this->_timeOffset + // User offset
         this->_currentEpoc + // Epoc returned by the NTP server
         ((millis() - this->_lastUpdate) / 1000); // Time since last update
}

int NTPClient::getDay() {
  return (((this->getEpochTime()  / 86400L) + 4 ) % 7); //0 is Sunday
}
int NTPClient::getHours() {
  return ((this->getEpochTime()  % 86400L) / 3600);
}
int NTPClient::getMinutes() {
  return ((this->getEpochTime() % 3600) / 60);
}
int NTPClient::getSeconds() {
  return (this->getEpochTime() % 60);
}

// 2022-04-18 Mod by @PaulskPt
// was: String NTPClient::getFormattedTime(unsigned long secs) {
String NTPClient::getFormattedTime(unsigned long secs, boolean disp_gmt) {
  unsigned long rawTime = secs ? secs : this->getEpochTime();

  if (disp_gmt)
  {
      if (rawTime)
      {
        Serial.print("getFormttedTime() rawTime = ");
        Serial.println(rawTime);
        if (this->_timeOffset > 0)
        {
            Serial.print("getFormattedTime() correcting rawTime. Subtracting timeOffset: ");
      Serial.println(this->_timeOffset);
            rawTime -= this->_timeOffset;  // e.g.: Lisbon DST is UTC + 3600 (1 hr), so we have to subtract to get UTC
        }
        else if (this->_timeOffset < 0)
            rawTime += this->_timeOffset;  // e.g.: Louisville, USA DTS is UTC - 4 hrs = - 4*3600 = 14400 secs, so we have to add 14400.
        if (rawTime < 0)
          rawTime = 0;
      }
  else
    Serial.print("getFormattedTime(): we will use local time");
  }
  // end-of-mod
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

// Based on https://github.com/PaulStoffregen/Time/blob/master/Time.cpp
// currently assumes UTC timezone, instead of using this->_timeOffset
// was: String NTPClient::getFormattedDate(unsigned long secs) {
String NTPClient::getFormattedDate(unsigned long secs, boolean disp_gmt) {
  unsigned long rawTime = (secs ? secs : this->getEpochTime()) / 86400L;  // in days
  unsigned long days = 0, year = 1970;
  uint8_t month;
  static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

  while((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0
  days=0;
  for (month=0; month<12; month++) {
    uint8_t monthLength;
    if (month==1) { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }
  String monthStr = ++month < 10 ? "0" + String(month) : String(month); // jan is month 1  
  String dayStr = ++rawTime < 10 ? "0" + String(rawTime) : String(rawTime); // day of month  
  // 2022-04-18 Mod by @PaulskPt
  if (secs)
  {
    Serial.print("secs = ");
    Serial.println(secs);
    if (this->_timeOffset > 0)
    {
        Serial.println("getFormattedDate() correcting rawTime. Subtracting timeOffset");
        secs -= this->_timeOffset;  // e.g.: Lisbon DST is UTC + 3600 (1 hr), so we have to subtract to get UTC
    }
    else if (this->_timeOffset < 0)
        secs += this->_timeOffset;  // e.g.: Louisville, USA DTS is UTC - 4 hrs = - 4*3600 = 14400 secs, so we have to add 14400.
    if (secs < 0)
      secs = 0;
  }
  int n = 0;
  if (this->_timeOffset != 0)
      n = this->_timeOffset / 3600;  // convert timeOsset to hours
  String tz_ltr = this->tz_nato((String)n);
  // was: return String(year) + "-" + monthStr + "-" + dayStr + "T" + this->getFormattedTime(secs ? secs : 0) + "Z";
  return String(year) + "-" + monthStr + "-" + dayStr + "T" + this->getFormattedTime(secs ? secs : 0, disp_gmt) + tz_ltr;
  // end-of-mod
}

void NTPClient::end() {
  this->_udp->stop();

  this->_udpSetup = false;
}

void NTPClient::setTimeOffset(int timeOffset) {
  this->_timeOffset     = timeOffset;
}

void NTPClient::setUpdateInterval(unsigned long updateInterval) {
  this->_updateInterval = updateInterval;
}

void NTPClient::sendNTPPacket() {
  // set all bytes in the buffer to 0
  memset(this->_packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  this->_packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  this->_packetBuffer[1] = 0;     // Stratum, or type of clock
  this->_packetBuffer[2] = 6;     // Polling Interval
  this->_packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  this->_packetBuffer[12]  = 0x49;
  this->_packetBuffer[13]  = 0x4E;
  this->_packetBuffer[14]  = 0x49;
  this->_packetBuffer[15]  = 0x52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  this->_udp->beginPacket(this->_poolServerName, 123); //NTP requests are to port 123
  this->_udp->write(this->_packetBuffer, NTP_PACKET_SIZE);
  this->_udp->endPacket();
}

void NTPClient::setEpochTime(unsigned long secs) {
  this->_currentEpoc = secs;
}

/**
* Return the NATO Timezone letter for the string timeOffset given
* Param String: tz (range "-12" to "+12"). "0" will return "Z" (GMT/UTC)
* Return String: NATO timezone letter
*/
String NTPClient::tz_nato(String tz){
    int le = tz.length();
    if (le > 3)
        return "?";

    if (le == 1 || le == 2 || le == 3)
    {
        int n = tz.toInt();
        if (n == 0)
            return "Z";
        else if (n < 0)
        {   // Westerly timezones
            n = abs(n);
            if (n > 12)
      n = 0;
      return tz_west[n];
    } 
    else if (n > 0)
    {  // Easterly timezones
          
            if (n > 12)
        n = 0;
      return tz_east[n];
        }
        else
            return tz_west[0];
    }
    else
        return tz_west[0];
}
