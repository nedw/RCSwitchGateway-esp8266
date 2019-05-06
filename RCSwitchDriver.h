#include "RCSwitch.h"


// SOFTAP
//
// If defined, use a standalone soft WiFi access point, else use an 
// external WiFi access point.  See also credentials.h.
//

#define SOFTAP

// RECEIVE_TEST
//
// This is used whenever you wish to reverse engineer the output from a mains switch remote control (see also
// comment for "switchGroup").  The output when a remote control button is pressed following the format below:
//
//  Decimal: <decimal> (24Bit) Binary: <binary> Tri-State: <tristate> PulseLength: <decimal> microseconds Protocol: <decimal>
//  Raw data: <comma separated decimals>
//
// The "<tristate>" part is important, and is a string with '0', '1' and 'F' (floating) representing the three states.
//

#define RECEIVE_TEST

//
// Receive/Transmit parameters
//

#define RCS_REPEAT_DELAY_MS     6       // Delay between repeat transmissions
#define RCS_REPEAT_COUNT        5       // Number of repeat transmissions
#define RCS_POWER_DELAY_MS      50      // Delay after lowering enable pin before transmitting
#define RCS_TRANSMIT_PIN        5       // GPIO of transmit pin
#define RCS_ENABLE_PIN          4       // GPIO of enable/power pin (active low)
#ifdef RECEIVE_TEST
#define RCS_RECEIVE_PIN         5       // GPIO of receive pin (RECEIVE_TEST only)
#else
#define RCS_RECEIVE_PIN         -1
#endif
#define RCS_PULSE_LENGTH_US     185     // Base pulse length (depends on remote)

constexpr int kMaxSockets = 5;    // includes special socket number 'SwitchDeviveAll' representing all sockets

class RCSwitchDriver {
public:

  struct Request {
    int socket;
    bool state;    
  };

  //
  // Configuration parameters:
  //
  //  repeatDelayMs         Delay between repeat transmissions (ms)
  //  repeatCount           Number of repeat transmissions
  //  powerDelayMs          Delay after pulling enablePin low before transmission starts (ms)
  //  transmitPin           GPIO of transmit pin
  //  enablePin             GPIO of transmitter enable pin (active low)
  //  receivePin            GPIO of receive pin (optional - set to -1 if unused)
  //  pulseLengthUs         Length of each bit (us)
  //  switchGroup           Address bits for transmission

  struct Config {
    int repeatDelayMs;
    int repeatCount;
    int powerDelayMs;
    int transmitPin;
    int enablePin;
    int receivePin;
    int pulseLengthUs;
    const char *switchGroup;
  };

  int init(const Config& config);
  void request(int socket, bool state);
  void process();
  void setSwitchState(int socket, bool state);

private:
  char* getCodeWordA_EtekCity(const char* sGroup, int nDevice, bool bStatus);

  volatile bool active_ = false;
  bool initted_ = false;
  Request request_;
  Config cfg_;
};

