#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "RCSwitchDriver.h"

//
// credentials.h contains WiFi ssid/password and remote control address information.
// It is of the form:
//
//  #ifndef _WIFI_CREDENTIALS_H_
//  #define _WIFI_CREDENTIALS_H_
//  
//  #ifdef SOFTAP
//  const char* ssid = "<soft ssid>";
//  const char* password = "<soft password>";
//  #else
//  const char* ssid = "<ssid>";
//  const char* password = "<password>";
//  #endif
//  
//  const char *switchGroup = "<remote control address>";
//  
//  #endif
//
//  "<remote control address> is a string of 5 tri-state characters ('0', '1' or 'F').
//

#include "credentials.h"     // requires SOFTAP (if defined)

//
// Definitions for RCSwitchDriver (front end to RCSwitch library).
//
// switchGroup:
//
// This is the unique address of the remote control hardcoded via the HS2260A-R4 address pins.
// I originally obtained this by capturing the remote control signal using a 433Mhz receiver
// connected to a USB logic analyser.  I used Sigrok and the Princeton datasheet to decode the
// signal from all the buttons.  I opened the remote to determine which Princeton chip was being
// used.
//
// However, I recommend enabling RECEIVE_TEST and pressing a key on the remote (I didn't have
// this working originally).  NOTE: that some of the cheap 433Mhz receivers are correspondingly
// poor quality; they may require you to hold the remote right up against them to get any form
// of useable signal.  This makes some of them a bit useless for anything other than reverse
// engineering.  The 433Mhz transmitter side seemed to work okay.
//
// Once you reach the stage of getting a "<tristate>" value (see comment against RECEIVE_TEST),
// then you can extract the first 5 characters and place them in "switchGroup".  Unfortunately,
// this can depend on the protocol used by the main plug.  I just know how the particular
// EtekCity remote that I have works, because of the time I spent looking at the datasheet
// and decoding the output.  Other mains plugs may vary.
//
extern const char *switchGroup;         // 5 character tri-state address, formed of '0', '1' or 'F' characters.

RCSwitchDriver rcSwitchDriver;          // RCSwitchDriver instance

//
// Definitions for HTTP server.  The following URLs are supported:
//
//  /switch/on/<socket num>     - Turn on <socket num> (e.g. "http://<ip>/switch/on/1").
//  /switch/off/<socket num>    - Turn off <socket num> (similar example to above).
//
// <socket num> is zero-based, and typically in the range 0-3.
//
constexpr int port = 80;
const char *uriOn = "/switch/on/";
const int uriOnLength = strlen(uriOn);
const char *uriOff = "/switch/off/";
const int uriOffLength = strlen(uriOff);
ESP8266WebServer server;

// This won't work with multiple socket requests

void handleRequest()
{
  Serial.println("handleRequest()");
  String s = "<html><body>URI: ";
  String uri = server.uri();
  s += uri + "<br>Args: ";
  int count = server.args();
  int i;
  for (i = 0 ; i < count ; ++i) {
    s += server.argName(i) + "=" + server.arg(i) + ", ";
  }

  s += "<br>Headers: ";
  count = server.headers();
  for (i = 0 ; i < count ; ++i) {
    s += server.headerName(i) + "=" + server.header(i) + ", ";
  }

  WiFiClient client = server.client();
  s += "<br>Remote IP/port: " + client.remoteIP().toString() + ":" + client.remotePort();
  s += "</body></html>";

  Serial.println(uri);

  int state = -1;
  int socket;
  if (uri.startsWith(uriOn)) {
    state = 1;
    socket = uri[uriOnLength] - '0';
  } else
  if (uri.startsWith(uriOff)) {
    state = 0;
    socket = uri[uriOffLength] - '0';
  }
  if (state != -1) {
    if (socket >= 0 && socket < kMaxSockets) {    
      Serial.printf("Switch socket %d %s\n", socket, state ? "on" : "off");
      // Not ideal - should queue requests here
      rcSwitchDriver.request(socket, state);
      server.send(200, "text/html", s);
    } else
      state = -1;
  }

  if (state == -1) {
    server.send(404, "text/html", "");
  }
}

void init_tcp()
{
  server.onNotFound(handleRequest);
  server.begin();  
}
void init_WiFi(void) {
#ifdef SOFTAP
  Serial.printf("Starting soft AP\n");
  WiFi.persistent(false);
  WiFi.softAP(ssid, password);
  IPAddress addr = WiFi.softAPIP();
  Serial.printf("IP Address ");
  Serial.println(addr);
#else
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf("IP Address ");
  Serial.println(WiFi.localIP());
#endif
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  delay(1000);
  Serial.println();
  Serial.println("setup()");
  Serial.println();

  RCSwitchDriver::Config cfg = {
    .repeatDelayMs  = RCS_REPEAT_DELAY_MS,
    .repeatCount    = RCS_REPEAT_COUNT,
    .powerDelayMs   = RCS_POWER_DELAY_MS,
    .transmitPin    = RCS_TRANSMIT_PIN,
    .enablePin      = RCS_ENABLE_PIN,
    .receivePin     = RCS_RECEIVE_PIN,             
    .pulseLengthUs  = RCS_PULSE_LENGTH_US,
    .switchGroup    = switchGroup
  };

  rcSwitchDriver.init(cfg);

#ifndef RECEIVE_TEST
  // WiFi/TCP does not seem to work work when receive test is enabled.
  // Presumably the many spurious GPIO interrupts generated by receiver
  // noise swamp out networking.  Perhaps it would work better on an
  // ESP32 rather than ESP8266.
  init_WiFi();
  init_tcp();
#endif
}  

void loop() {
#ifndef RECEIVE_TEST
  server.handleClient();
#endif
  rcSwitchDriver.process();
}
