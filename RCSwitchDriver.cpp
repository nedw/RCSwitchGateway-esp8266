//
// RCSwitchDriver - a front end to RCSwitch library for this project.
//

#include "RCSwitchDriver.h"

RCSwitch rcSwitch;

const int SwitchDeviceAll = 4;        // Device 4 represents all sockets (i.e. 0-3)

int RCSwitchDriver::init(const Config& cfg) {

  cfg_ = cfg;
  rcSwitch.setProtocol(1);
  rcSwitch.setPulseLength(cfg_.pulseLengthUs);
  rcSwitch.enableTransmit(cfg_.transmitPin);

  if (cfg.receivePin != -1) {
    const int receiveInterrupt = digitalPinToInterrupt(cfg.receivePin);
    Serial.printf("Receive pin %d = interrupt %d\n", cfg.receivePin, receiveInterrupt);
    rcSwitch.enableReceive(receiveInterrupt);
  }

  // Init transmitter to off state
  pinMode(cfg_.enablePin, OUTPUT);
  digitalWrite(cfg_.enablePin, HIGH);   // Disable power to transmitter
  // Relying on enableTransmit() to have configured as OUTPUT                                          
  digitalWrite(cfg_.transmitPin, LOW);  // Set transmit input low on transmitter

  initted_ = true;
  return 0;
}

//
// EtekCity tristate code word (12 tri-state bits):
//
//    AAAAA BBBBB CC
//
// Where:
//
// A = Switch Group
//    (hardcoded in remote)
//
// B = Switch Device
//    0   = FFF01
//    1   = FFF10
//    2   = FF100
//    3   = F1F00
//    All = 1FF00 
//
// C = Command
//    On  = 01
//    Off = 10
//

char* RCSwitchDriver::getCodeWordA_EtekCity(const char* sGroup, int nDevice, bool bStatus) {
  static char sReturn[13];
  int nReturnPos = 0;

  if (sGroup == NULL || nDevice < 0 || nDevice >= kMaxSockets)
    return NULL;

  for (int i = 0; i < 5; i++) {
    if (!sGroup[i])
      return NULL;
    else
      sReturn[nReturnPos++] = sGroup[i];
  }

  for (int i = 0; i < 3; i++) {
    sReturn[nReturnPos++] = (nDevice == 4 - i) ? '1' : 'F';
  }

  for (int i = 0; i < 2; i++) {
    sReturn[nReturnPos++] = (nDevice == 1 - i) ? '1' : '0';
  }

  sReturn[nReturnPos++] = bStatus ? '0' : '1';
  sReturn[nReturnPos++] = bStatus ? '1' : '0';

  sReturn[nReturnPos] = '\0';
  return sReturn;
}

void RCSwitchDriver::setSwitchState(int socket, bool state)
{
  digitalWrite(cfg_.enablePin, LOW);   // Enable power to transmitter
  delay(cfg_.powerDelayMs);

  int n = cfg_.repeatCount;
  while (n-- > 0) {
    Serial.printf("SwitchGroup '%s', Socket %d, state %d\n", cfg_.switchGroup, socket, state);
    rcSwitch.sendTriState(getCodeWordA_EtekCity(cfg_.switchGroup, socket, state));
    delay(cfg_.repeatDelayMs);
  }

  delay(cfg_.powerDelayMs);
  digitalWrite(cfg_.enablePin, HIGH);   // Disable power to transmitter
}

void RCSwitchDriver::request(int socket, bool state)
{
  if (!active_) {               // Need to queue here if already active
    request_.socket = socket;
    request_.state = state;  
    active_ = true;
  }
}

//
// Logging code taken from rc-switch project
//

static const char* bin2tristate(const char* bin) {
  static char returnValue[50];
  int pos = 0;
  int pos2 = 0;
  while (bin[pos]!='\0' && bin[pos+1]!='\0') {
    if (bin[pos]=='0' && bin[pos+1]=='0') {
      returnValue[pos2] = '0';
    } else if (bin[pos]=='1' && bin[pos+1]=='1') {
      returnValue[pos2] = '1';
    } else if (bin[pos]=='0' && bin[pos+1]=='1') {
      returnValue[pos2] = 'F';
    } else {
      return "not applicable";
    }
    pos = pos+2;
    pos2++;
  }
  returnValue[pos2] = '\0';
  return returnValue;
}

static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
  static char bin[64]; 
  unsigned int i=0;

  while (Dec > 0) {
    bin[32+i++] = ((Dec & 1) > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j< bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[ 31 + i - (j - (bitLength - i)) ];
    } else {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';
  
  return bin;
}

static void output(unsigned long decimal, unsigned int length, unsigned int delay, unsigned int* raw, unsigned int protocol) {

  if (decimal == 0) {
    Serial.print("Unknown encoding.");
  } else {
    const char* b = dec2binWzerofill(decimal, length);
    Serial.print("Decimal: ");
    Serial.print(decimal);
    Serial.print(" (");
    Serial.print( length );
    Serial.print("Bit) Binary: ");
    Serial.print( b );
    Serial.print(" Tri-State: ");
    Serial.print( bin2tristate( b) );
    Serial.print(" PulseLength: ");
    Serial.print(delay);
    Serial.print(" microseconds");
    Serial.print(" Protocol: ");
    Serial.println(protocol);
  }
  
  Serial.print("Raw data: ");
  for (unsigned int i=0; i<= length*2; i++) {
    Serial.print(raw[i]);
    Serial.print(",");
  }
  Serial.println();
  Serial.println();
}

void RCSwitchDriver::process()
{
  if (!initted_)
    return;

  if (active_) {
    setSwitchState(request_.socket, request_.state);
    active_ = false;
  }
  
  if (cfg_.receivePin != -1 && rcSwitch.available()) {
    output(rcSwitch.getReceivedValue(), rcSwitch.getReceivedBitlength(), rcSwitch.getReceivedDelay(), rcSwitch.getReceivedRawdata(),rcSwitch.getReceivedProtocol());
    rcSwitch.resetAvailable();
  }
}
