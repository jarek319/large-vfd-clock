#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
extern "C" {
#include "user_interface.h"
}

#define TOPCOLON 14
#define BOTTOMCOLON 12
#define DIN 5
#define CLK 4
#define LOAD 0
#define BLANK 2

// NTP code adapted from ESP8266 Example code in Arduino core
char ssid[] = "ssid";  //  your network SSID (name)
char pass[] = "pass";       // your network password
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;   // A UDP instance to let us send and receive packets over UDP
const int timeZone = -4;

byte digitArray[] = {
  0b11101101,
  0b00101000,
  0b01110101,
  0b01111100,
  0b10111000,
  0b11011100,
  0b11011101,
  0b01101000,
  0b11111101,
  0b11111100,
  0b11111001,
  0b10011101,
  0b11000101,
  0b00111101,
  0b11010101,
  0b11010001
};

void drawDigits() {
  byte digits[4] = { 0, 0, 0, 0 };
  if ( hour() < 10 ) digits[ 3 ] = digitArray[ 0 ];
  else digits[ 3 ] = digitArray[ hour() / 10 ];
  digits[ 2 ] = digitArray[ hour() % 10 ];
  if ( minute() < 10 ) digits[ 1 ] = digitArray[ 0 ];
  else digits[ 1 ] = digitArray[ minute() / 10 ];
  digits[ 0 ] = digitArray[ minute() % 10 ];

  for ( int i = 0; i < 4; i++ ) {
    digitalWrite( LOAD, LOW );
    for ( int j = 0; j < 8; j++ ) {
      if ( i < 2 ) {
        if ( 1 & ( digits[ i ] >> j )) digitalWrite( DIN, HIGH );
        else digitalWrite( DIN, LOW );
      }
      else {
        if ( 1 & ( digits[ i ] >> ( 7 - j ))) digitalWrite( DIN, HIGH );
        else digitalWrite( DIN, LOW );
      }
      digitalWrite( CLK, HIGH );
      digitalWrite( CLK, LOW );
    }
    digitalWrite( LOAD, HIGH );
  }
}

void setup() {
  pinMode( TOPCOLON, OUTPUT );
  pinMode( BOTTOMCOLON, OUTPUT );
  pinMode( DIN, OUTPUT );
  pinMode( CLK, OUTPUT );
  pinMode( LOAD, OUTPUT );
  pinMode( BLANK, OUTPUT );
  digitalWrite( DIN, LOW );
  digitalWrite( CLK, LOW );
  digitalWrite( LOAD, LOW );
  digitalWrite( BLANK, LOW );
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  udp.begin(localPort);
  setSyncProvider(getNtpTime);
}

time_t getNtpTime() {
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  WiFi.hostByName( ntpServerName, timeServerIP );  //get a random server from the pool
  sendNTPpacket( timeServerIP ); // send an NTP packet to a time server
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
void loop() {
  static time_t prevDisplay = 0; // when the digital clock was displayed
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      drawDigits();
    }
    if ( second() % 2 ) {
      digitalWrite( TOPCOLON, HIGH );
      digitalWrite( BOTTOMCOLON, HIGH );
    }
    else {
      digitalWrite( TOPCOLON, LOW );
      digitalWrite( BOTTOMCOLON, LOW );
    }
  }
}
