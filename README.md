# Gateway for Controlling 433Mhz Mains Socket
## Description
This ESP8266 project implements a HTTP server for turning on and off a set of Etekcity 433Mhz mains sockets via a 433Mhz transmit module.  These mains sockets are based around the ubiquitous Princeton PT2260/70 series remote control encoder/decoder modules.  The project makes use of the *rc-switch* project to transmit the appropriately formatted packets.  This can be found at:

`https://github.com/sui77/rc-switch.git`.

The project implements a HTTP server with simple URLs that are used to switch numbered sockets on and off:

```
http://<ip address>/switch/on/<socket>
http://<ip address>/switch/off/<socket>
```
`<socket>` is a zero-based number typically in the range 0-3, and `<ip address>` is the WiFi IP address, obtained from the serial output.The project can be configured to implement either a local soft WiFi access point, or to use an external access point.

The project can also be configured to run in one of two modes:

1.  Transmit Mode.  This is the normal mode of operation that is used to control the sockets via a 433Mhz transmitter.
2.  Receive Mode.  This is used to reverse engineer the transmissions from the remote control using a 433Mhz receiver.  This is primarily to obtain the base address of the sockets to configure into the project.

The configuration of the above two modes is described in the next section.

## Getting up and running

This project has uses simple hardware - 433Mhz transmitters and receivers and off-the-shelf Etekcity remote mains sockets - but has a somewhat involve setup due to having to obtain the mains socket addresses.  The description in the next section assumes the use of the PT2260/70 series decoder/encoder modules.  Unfortunately, there are some variants of these modules that use different numers of address code words.  This project assumes the use of the variant used in the Etekcity sockets (HS2260A-R4).  This was discovered by carefully taking apart the remote control casing and looking at the encoder chip.

### Obtaining the Mains Socket Addresses
The addresses of the sockets that were used has
been hardcoded into this project.  The addresses were obtained by capturing the 433Mhz transmissions of the various buttons on the supplied remote control unit and analysing them in `Sigrok`.  A 433Mhz receiver module was used for this, with the Hobby Components 8ch Logic Analyzer used to capture the transmissions.  The socket addresses could then be decoded from the captures and the datasheet.

Fortunately, there is a (slightly) easier way to obtain the addresses.  `rc-switch` already has a receive decode facility which can form the basis of any capturing and decoding of addresses.

First, switch the project into "receive" mode by un-commenting the following define in`RCSwitchDriver.h`:

`#define RECEIVE_TEST` 

Next, attach the 433Mhz receiver to a suitable GPIO pin and modify the `RCS_RECEIVE_PIN` define in `RCSwitchGateway.h` accordingly.

You can then press the "on" buttons on the remote control for the various sockets (there are typically 4) and you should get output in the following form:

`Decimal: <decimal> (24Bit) Binary: <binary> Tri-State: <tristate> PulseLength: <decimal> microseconds Protocol: <decimal>`

`Raw data: <comma separated decimals>`

There are two important pieces of information here:

1. The `<tristate>` part is is a string composed of characters '0', '1' and 'F' (floating).  This represents the (tristate rather than binary) code address of the mains sockets.  The first 5 characters need to be placed into the `switchGroup` variable defined `credentials.h` (but see text below).
2.  The `PulseLength: <decimal> microseconds` part needs to be used to setup `RCS_PULSE_LENGTH_US` in `RCSwitchDriver.h`

NOTE: some of the cheap 433Mhz receivers are correspondingly
poor quality; they may require you to hold the remote right up against them to get any form of useable signal.  This makes some of them a bit useless for anything other than reverse engineering.  The cheap 433Mhz transmitter side that I used, on the other hand, seemed to work okay with the mains sockets.

Unfortunately,
the number of characters to extract from the beginning of the `<tristate>` value depends on the protocol and PT2260 encoder chip variant used by the main plug.  I just know that the particular EtekCity remote that I have works this way, because of the time I spent looking at the datasheet and decoding the output.  Other mains plugs may vary in the number of tristate words used for addressing.

### Setting up Transmit and Enable GPIO pins

Assuming you have obtained the socket addresses as described in the previous section, the next step is to configure the "Transmit" and "Enable" GPIO pin numbers.  As the names suggest, the "Transmit" pin is used for the data transmission, and the "Enable" pin is used as an active low signal to enable the transmitter.  In my project hardware, the Enable pin is used to enable power to the transmit module.  Hence, there is a slight power-up delay in the code after setting the pin active low.  This pin is optional if you don't have such a setup.

The GPIO pin numbers should be setup by modifying `RCS_TRANSMIT_PIN` and `RCS_ENABLE_PIN` in `RCSwitchDriver.h`.

### Setting up WiFi Credentials and Mains Socket Base Address

The file `credentials.h` (not included in source) contains the definition of the WiFi
SSID/password and base address of the mains socket.  These are considered to
be sensitive information, hence they are separated out into a separate file.  The file
takes the following form:

```
#ifndef _WIFI_CREDENTIALS_H_
#define _WIFI_CREDENTIALS_H_

#ifdef SOFTAP
const char* ssid = "<local SSID>";
const char* password = "<local password>";
#else
const char* ssid = "<external SSID>";
const char* password = "<external password";
#endif

const char *switchGroup = "<5 character tristate address string>";       // formed of '0', '1' or 'F'

#endif
```

The `SOFTAP` definition determines whether the ESP8266 implements a local soft WiFi access point or uses an external WiFi access point.  If a local access point is required, this should be defined in `RCSwitchDriver.h`.  The `ssid` and `password` in the appropriate section of the file should then be setup.

The `switchGroup` should be setup as described in the previous section.

## Summary

This project has uses simple hardware - 433Mhz transmitters and receivers - but has a somewhat involve setup due to having to obtain the mains socket addresses.

The only way to do this reasonably is to use the receive mode and capture the output of the remote control when pressing the "on" buttons.