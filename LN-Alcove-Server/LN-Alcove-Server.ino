// LN Alcover LN weight and compressed air Readout
// V.E. Guiseppe
// 5 July 2017
// Modified from:

/////////////////
// Sample Arduino Json Web Server
// Created by Benoit Blanchon.
// Heavily inspired by "Web Server" from David A. Mellis and Tom Igoe
////////////////
//
////////////////
//Udp NTP Client
//
// Get the time from a Network Time Protocol (NTP) time server
// Demonstrates use of UDP sendPacket and ReceivePacket
// For more on NTP time servers and the messages needed to communicate with them,
// see http://en.wikipedia.org/wiki/Network_Time_Protocol
//
// created 4 Sep 2010
// by Michael Margolis
// modified 9 Apr 2012
// by Tom Igoe
///////////////////
//
//PX309-100G5V on analog A2
// found P=100 at adc 847; P=80 at 690
// P = a0 + a1 V
// a0 = -7.9
// a1 = 0.127

#include <ArduinoJson.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <SPI.h>



//byte mac[] = {0xFE, 0xED, 0xDE, 0xAD, 0xBE, 0xEF}; // original alcove
byte mac[] = {0x90, 0xA2, 0xDA, 0x11, 0x0F, 0xEB}; // ethernet 2 replacement for alcove
//IPAddress ip(151, 159, 226, 98); // original alcove
IPAddress ip(151, 159, 226, 112); // ethernet 2 replacement for alcove
EthernetServer server8080(8080);
EthernetServer server(80);

unsigned int localPort = 8888;       // local port to listen for UDP packets
char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

bool readRequest(EthernetClient& client8080) {
  bool currentLineIsBlank = true;
  while (client8080.connected()) {
    if (client8080.available()) {
      char c = client8080.read();
      if (c == '\n' && currentLineIsBlank) {
        return true;
      } else if (c == '\n') {
        currentLineIsBlank = true;
      } else if (c != '\r') {
        currentLineIsBlank = false;
      }
    }
  }
  return false;
}



int value;
double ConvertValue;
unsigned long cycle = 60; // in sec. time between ORCA polls 
unsigned long lastTime = 0;
unsigned long theTime;




void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac); //with ,ip was breaking the time server
  server8080.begin();
  server.begin();
  Udp.begin(localPort);
  Serial.println(Ethernet.localIP());
}


void loop() {
  EthernetClient client8080 = server8080.available();
  if (client8080) {
    bool success = readRequest(client8080);
    if (success) {
      theTime = GetTime(lastTime);
      Serial.println(theTime);
      lastTime = theTime;
      // Use https://bblanchon.github.io/ArduinoJson/assistant/ to
      // compute the right size for the buffer
      StaticJsonBuffer<500> jsonBuffer;
      JsonArray& list = jsonBuffer.createArray();
      /////////////////
      //sensor 0
      JsonObject& sensor0 = jsonBuffer.createObject();
      sensor0["description"] = "LN Scale 0";
      //average 20 to smooth the data
      ConvertValue=0;
      for(int i=0;i<20;i++){
        value = analogRead(A0);
        ConvertValue += round((double)value*1.21743-472.771);
        delay(10);
      }
      ConvertValue=round(ConvertValue/20);
      sensor0["value"] = ConvertValue;
      sensor0["time"] = theTime;
      sensor0["unit"] = "lbs";
      list.add(sensor0);
      ///////////////////////
      //sensor 1
      JsonObject& sensor1 = jsonBuffer.createObject();
      sensor1["description"] = "LN Scale 1";
      //average 20 to smooth the data
      ConvertValue=0;
      for(int i=0;i<20;i++){
        value = analogRead(A1);
        ConvertValue += round((double)value*1.21805-470.586);
        delay(10);
      }
      ConvertValue=round(ConvertValue/20);
      sensor1["value"] = ConvertValue;
      sensor1["time"] = theTime;
      sensor1["unit"] = "lbs";
      list.add(sensor1);
      ///////////////////////
      //sensor 2
      JsonObject& sensor2 = jsonBuffer.createObject();
      sensor2["description"] = "Compressed air";
      //average 20 to smooth the data
      ConvertValue=0;
      for(int i=0;i<20;i++){
        value = analogRead(A2);
        ConvertValue += round((double)value*0.127-7.9);
        delay(10);
      }
      ConvertValue=round(ConvertValue/20);
      sensor2["value"] = ConvertValue;
      sensor2["time"] = theTime;
      sensor2["unit"] = "psi";
      list.add(sensor2);
      //
      writeResponse(client8080, list);
    }
    delay(1);
    client8080.stop();
  }
  //NOW RUN THE old LN weight werver
  EthernetClient client = server.available();
  if (client) {
   Serial.println("new client");
   // an http request ends with a blank line
   boolean currentLineIsBlank = true;
   while (client.connected()) {
     if (client.available()) {
       char c = client.read();
       Serial.write(c);
       // if you've gotten to the end of the line (received a newline
       // character) and the line is blank, the http request has ended,
       // so you can send a reply
       if (c == '\n' && currentLineIsBlank) {
         // send a standard http response header
         client.println("HTTP/1.1 200 OK");
         client.println("Content-Type: text/html");
         client.println("Connection: close");  // the connection will be closed after completion of the response
         client.println("Refresh: 5");  // refresh the page automatically every 5 sec
         client.println();
         client.println("<!DOCTYPE HTML>");
         client.println("<html>");
         // output the value of each analog input pin
         for (int analogChannel = 0; analogChannel < 2; analogChannel++) {
           int sensorReading = analogRead(analogChannel);
           client.print("Scale ");
           client.print(analogChannel);
           client.print(" voltage ");
           client.print(sensorReading);
           client.println("<br />");
         }
         client.println("</html>");
         break;
       }
       if (c == '\n') {
         // you're starting a new line
         currentLineIsBlank = true;
       } else if (c != '\r') {
         // you've gotten a character on the current line
         currentLineIsBlank = false;
       }
     }
   }
   // give the web browser time to receive the data
   delay(1);
   // close the connection:
   client.stop();
   Serial.println("client disconnected");
   Ethernet.maintain();
 }
}

void writeResponse(EthernetClient& client8080, JsonArray& json) {
  client8080.println("HTTP/1.1 200 OK");
  client8080.println("Content-Type: application/json");
  client8080.println("Connection: close");
  client8080.println();
  json.prettyPrintTo(client8080);
}


// send an NTP request to the time server at the given address
void sendNTPpacket(char* address)
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
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

unsigned long GetTime(unsigned long last){
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);
  unsigned long timestamp = 0;
  if ( Udp.parsePacket() ) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into Unix time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long unix = secsSince1900 - seventyYears;
    timestamp = unix;
  }
  else{
    timestamp = last+cycle;
  }
  return timestamp;
}
