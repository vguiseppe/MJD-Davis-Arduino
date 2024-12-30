

// Purge Heat Exchanger Temp Sensor Readout
// V.E. Guiseppe
// 4 Sept 2017
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
#include <Event.h>
#include <Timer.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>


byte mac[] = {0x90, 0xA2, 0xDA, 0x11, 0x0F, 0x2F}; //purge arduino
//byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0xC8, 0x35}; //Davis Alrm arduino
//IPAddress ip(151, 159, 226, 110); //purge arduino at SURF
//IPAddress ip(192, 168, 0, 23); //rental house
//IPAddress ip(151, 159, 226,); //alarm arduino at SURF
EthernetServer server(80);

unsigned int localPort = 8888;       // local port to listen for UDP packets

char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

bool readRequest(EthernetClient& client) {
  bool currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
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
double ConvertValue=0;
unsigned long cycle = 60; // in sec. time between ORCA polls 
unsigned long lastTime = 0;
unsigned long theTime;

//LN interlock
int AMIch=3; //AMI closed contact input
int IOch=4; //iBoot IO input
bool tripped=0;
int relay=1;
Timer t;


void setup() {
  //analogReference(INTERNAL);
  Serial.begin(9600);
  delay(100);
  Ethernet.begin(mac);
  delay(100);
  server.begin();
  delay(100);
  Udp.begin(localPort);
  delay(100);
  Serial.println(Ethernet.localIP());
  delay(100);
  analogReference(INTERNAL);
  //LN interlock
  pinMode(AMIch,INPUT_PULLUP);
  pinMode(IOch,OUTPUT);
  digitalWrite(IOch,HIGH);
  //int tick = t.every(10000,TripReset); //trying t.after instead of reg freq.
}


void loop() {
  //keep counter going
  t.update();
  delay(1000); //breather
  //FirstCheck AMI Alarm
  //Serial.println("checking AMI");
  relay = digitalRead(AMIch);
  Serial.println("relay is");
  Serial.println(relay);
  if(relay == 0 && tripped == 0){
    //relay is tripped for the first time
    Serial.println("relay is now tripped: IO going LOW");
    digitalWrite(IOch,LOW);
    tripped=1;
    int tick = t.after(600000,TripReset); //check again later
  }
  // want it to latch, so dont check it it cleared on its own.
  // wait for the time check
  EthernetClient client = server.available();
  if (client) {
    bool success = readRequest(client);
    if (success) {
      //get time
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
      sensor0["description"] = "shield purge water temp";
      //average 20 to smooth the data
      ConvertValue=0;
      for(int i=0;i<20;i++){
        value = analogRead(A0);
        ConvertValue += (double)value*1.1/1023*100;
        delay(10);
      }
      ConvertValue=round(ConvertValue/20);
      sensor0["value"] = ConvertValue;
      sensor0["time"] = theTime;
      sensor0["unit"] = "F";
      list.add(sensor0);
      ///////////////////////
      //sensor 1
      JsonObject& sensor1 = jsonBuffer.createObject();
      sensor1["description"] = "GB purge water temp";
      //average 20 to smooth the data
      ConvertValue=0;
      for(int i=0;i<20;i++){
        value = analogRead(A1);
        ConvertValue += round((double)value*1.1/1023*100);
        delay(10);
      }
      ConvertValue=round(ConvertValue/20);
      sensor1["value"] = ConvertValue;
      sensor1["time"] = theTime;
      sensor1["unit"] = "F";
      list.add(sensor1);
      ////////////////
      //sensor 2
      JsonObject& sensor2 = jsonBuffer.createObject();
      sensor2["description"] = "AMI LN Hi Alarm";
      sensor2["value"] = tripped;
      sensor2["time"] = theTime;
      sensor2["unit"] = "bool";
      list.add(sensor2);
      ////////////////
      writeResponse(client, list);
    }
    delay(10);
    client.stop();
  }
  
}

void writeResponse(EthernetClient& client, JsonArray& json) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  json.prettyPrintTo(client);
}


// send an NTP request to the time server at the given address
void sendNTPpacket(char* address) {
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
void TripReset(){
  if(tripped){
    //still check relay so that reset time can be reset
    Serial.println("Resetting tripped: IO going HI");
    tripped = 0;
    digitalWrite(IOch,HIGH);
  }
}

