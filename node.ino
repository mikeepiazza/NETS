/*
Node.ino
*/

#include <SPI.h> // needed for Arduino versions later than 0018
#include <Ethernet2.h>
#include <EthernetUdp2.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008

const char MAC[] = {0x2C, 0xF7, 0xF1, 0x08, 0x24, 0x5C}; // from Ethernet Shield
const int localPort = 5002; // local UDP port
const IPAddress server(192, 168, 0, 1); // server has static-IP
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; // buffer to hold incoming packet
EthernetUDP UDP; // UDP object
// create enum to specify different packet types, RX and TX

// IRIG 10.3.9.1.2 UDP Transfer Header
// Figure 10-4 (non-segmented data)
const char versionAndTypeOfMessage = 0x01; // bits 3 to 0: Version 1, 7 to 4: Full packets
char UDPMessageSequenceNumber[3]; // 24 bits, roll over at 0xFFFFFF
// channelID, sequenceNumber, segmentOffset in segmented packets only
// Total: 4B

// IRIG 10.6.1.1 Packet Header:
const char packetSyncPattern[2] = {0xEB, 0x25}; // a
char channelID[2] = {NULL, NULL}; // b, *should be easy to change
char packetLength[4] = {NULL, NULL, NULL, NULL}; // c
char dataLength[4] = {NULL, NULL, NULL, NULL}; // d
const char dataTypeVersion = 0x05; // e, RCC 106-11
char sequenceNumber = 0x00; // f, rolls over at 255
char packetFlags = 0x02; // g, use 16-bit checksum
char dataType = 0x09; // h, 0x09 for data packets, 0x11 for time packets
char relativeTimeCounter[6] = {NULL, NULL, NULL, NULL, NULL, NULL}; // i
char headerChecksum[2] = {NULL, NULL}; // j
// Total: 20B

// IRIG 10.6.1.3 Packet Body:
char channelSpecificData[4] = {0x00, 0x00, 0x5F, 0x08}; // a, set from requirements document, *should be easy to change
char intraPacketTimeStamp[8] = {0x00, 0x00, NULL, NULL, NULL, NULL, NULL, NULL}; // b, use 48-bit relative time count
char intraPacketDataHeader[2] = {0xF0, 0x00}; // c, *should be easy to change
char data[600]; // d, v59 equivalent, (200*2.5B current measurements, 10*10B status reports)
// Total: 614B@2000SPS, 314B@1000SPS, 164B@500SPS, 74B@200SPS

// IRIG 10.6.1.4 Packet Trailer:
const char filler[1] = {0x00}; // a, 2B filler maintains 32B-width, total: 640B
char dataChecksum; // b
// Total: 641B

void registrar() { // gets initial config from controller
    while(channelID[1] == NULL) { // no channelID? keep trying
      UDP.beginPacket(server, localPort); // initialize packet of data
      UDP.write(0x01); // request initial config
      UDP.write(MAC); // pass MAC address
      UDP.endPacket(); // send packet
      delay(5000); // give server time to respond
      packetHandler(); // check for initial config packet
    }
}

void packetHandler() { // process packets
  int packetSize = UDP.parsePacket(); // must be called before read()
  if (packetSize) { // packet waiting?
    UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE); // read packet into buffer
    for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) { // for test only
      Serial.println(byte(packetBuffer[i]), BIN); // write contents to serial monitor
    }
    if (packetBuffer[0] == 0x01) { // initial configuration received
      // create enum to specify what is in initial config
      channelID[1] = packetBuffer[1]; // set channel
      // hardware config, group assignment, commands etc.
    }
    else if (packetBuffer[0] == 0x02) { // command packets
      // implement commands
      // send ACK
    }
  }
}

void setup() { // configure network
  Serial.begin(115200); // serial baud-rate, for testing
  if (Ethernet.begin(MAC) == 0) { // unable to get IP from DHCP server?
    Serial.println("Failed to configure Ethernet using DHCP");
    while(true); // infinite loop, hard-reset when network becomes available
  }
  UDP.begin(localPort); // start UDP
  registrar(); // get intial config
}

void loop() {
  // main
  Serial.print("I haz an ID: "); Serial.println(channelID);
  delay(999999999);
}
