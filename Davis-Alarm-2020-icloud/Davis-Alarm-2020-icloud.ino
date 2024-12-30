/*
   Email client sketch for IDE v1.0.5 and w5100/w5200
   Posted 7 May 2015 by SurferTim
*/
 
#include <SPI.h>
#include <Ethernet2.h>
 
// this must be unique
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0xC8, 0x35}; //surf
//byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x78, 0x42}; //home
// change network settings to yours (IS THIS EVEN NEEDED?)
// SURF
//IPAddress ip( 151, 159, 226, 109 );    
//IPAddress gateway( 151, 159, 226, 65 );
//IPAddress subnet( 255, 255, 255, 192 );
// HOME
//IPAddress ip( 10, 0, 0, 17 );
 
char server[] = "66.228.43.14";
int port = 2525;
// digital imput channel 
int Ch = 3;
bool tripped=0;


char RCPTAddress1[50] = "RCPT To: <guiseppe@sc.com>";
char RCPTAddress2[50] = "RCPT To: <CabotAnn.Christofferson@sdsmt.edu>";
char RCPTAddress3[50] = "RCPT To: <5052313328@vtext.com>";
char RCPTAddress4[50] = "RCPT To: <6053902693@vtext.com>";
char RCPTAddress5[50] = "RCPT To: <sanfordlabscience@gmail.com>";
char ToAddress1[50] = "To: <guiseppe@sc.com>";
char ToAddress2[50] = "To: <CabotAnn.Christofferson@sdsmt.edu>";
char ToAddress3[50] = "To: <5052313328@vtext.com>";
char ToAddress4[50] = "To: <6053902693@vtext.com>";
char ToAddress5[50] = "To: <sanfordlabscience@gmail.com>";
 
EthernetClient client;
 
void setup()
{
  Serial.begin(9600);
  //pinMode(4,OUTPUT);
  //digitalWrite(4,HIGH);
  //Ethernet.begin(mac, ip, gateway, gateway, subnet);
  Ethernet.begin(mac); //simpler dont need IP
  delay(2000);
  //Input relay
  pinMode(Ch, INPUT_PULLUP);
  Serial.println(Ethernet.localIP());
  Serial.println("ready");
}
 
void loop()
{
  // read the input pin:
  Serial.println("checking");
  Serial.println("tripped is ");
  Serial.println(tripped);
  Serial.println("relay is ");
  int relay = digitalRead(Ch);
  Serial.println(relay);
  //delay(3000);
  if(relay == 0 && tripped == 0){
     //relay is tripped
     Serial.println("relay is tripped");
     char EmailMessage[100] = "There is a general facility alarm at the SURF Davis Campus";
     sendEmail(EmailMessage,RCPTAddress1,ToAddress1);
     sendEmail(EmailMessage,RCPTAddress2,ToAddress2);
     sendEmail(EmailMessage,RCPTAddress3,ToAddress3);
     sendEmail(EmailMessage,RCPTAddress4,ToAddress4);
     sendEmail(EmailMessage,RCPTAddress5,ToAddress5);

     //don;t send again until it clears
     tripped=1;
  }
  if(relay == 1 && tripped == 1){
     //relay is cleared
     Serial.println("relay is cleared");
     char EmailMessage[100] = "The general facility alarm at the SURF Davis Campus has cleared";
     sendEmail(EmailMessage,RCPTAddress1,ToAddress1);
     sendEmail(EmailMessage,RCPTAddress2,ToAddress2);
     sendEmail(EmailMessage,RCPTAddress3,ToAddress3);
     sendEmail(EmailMessage,RCPTAddress4,ToAddress4);
     sendEmail(EmailMessage,RCPTAddress5,ToAddress5);
     //don't send again until it trips
     tripped=0;
  }
  delay(1000);
}

 
byte sendEmail(char message[100], char RCPTaddress[100], char TOaddress[100])
{
  byte thisByte = 0;
  byte respCode;
 
  if(client.connect(server,port) == 1) {
    Serial.println(F("connected"));
  } else {
    Serial.println(F("connection failed"));
    return 0;
  }
 
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending hello"));
// replace 1.2.3.4 with your Arduino's ip 151, 159, 226, 109
  client.println("EHLO 151.159.226.109");
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending auth login"));
  client.println("auth login");
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending User"));
// Change to your base64 encoded user
  client.println("bWpk");
 
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending Password"));
// change to your base64 encoded password
  client.println("ZEhRMWF6RnBNakJwTm01bw==");
 
  if(!eRcv()) return 0;
 
// change to your email address (sender)
  Serial.println(F("Sending From"));
  client.println("MAIL From: nucontrol@icloud.com");
  if(!eRcv()) return 0;
 
// change to recipient address
  Serial.println(F("Sending To"));
  client.println(RCPTaddress);
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending DATA"));
  client.println("DATA");
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending email"));
 
// change to recipient address
 client.println(TOaddress); 
 
// change to your address
 client.println("From: MJD <nucontrol@icloud.com>");
 
  client.println("Subject: GENERAL FACILITY ALARM\r\n");
 
  client.println(message);
 
  client.println(".");
 
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending QUIT"));
  client.println("QUIT");
  if(!eRcv()) return 0;
 
  client.stop();
 
  Serial.println(F("disconnected"));
 
  return 1;
}
 
byte eRcv()
{
  byte respCode;
  byte thisByte;
  int loopCount = 0;
 
  while(!client.available()) {
    delay(1);
    loopCount++;
 
    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }
 
  respCode = client.peek();
 
  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }
 
  if(respCode >= '4')
  {
    efail();
    return 0;  
  }
 
  return 1;
}
 
 
void efail()
{
  byte thisByte = 0;
  int loopCount = 0;
 
  client.println(F("QUIT"));
 
  while(!client.available()) {
    delay(1);
    loopCount++;
 
    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return;
    }
  }
 
  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }
 
  client.stop();
 
  Serial.println(F("disconnected"));
}
