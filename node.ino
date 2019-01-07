/*
  node.ino: collects data and reports over ethernet
*/

#include <Ethernet.h>
#include <EthernetUdp.h>

// network config
byte macAddress[] = {0x2C, 0xF7, 0xF1, 0x08, 0x24, 0x5C}; // MAC address
IPAddress remoteIP(192, 168, 0, 1); // controller IP address
unsigned int localPort = 8888; // node UDP port
unsigned int remotePort = 8888; // controller UDP port
EthernetUDP Udp; // object allows UDP read/write
char rxBuffer[UDP_TX_PACKET_MAX_SIZE]; // receive buffer = 24B
char txBuffer[UDP_TX_PACKET_MAX_SIZE]; // transmit buffer = 24B

// typedefs for readability
typedef enum {STRAY_i, FIRED_i, LOWCURRENTSTATUS_i, HIGHCURRENTCOMMAND_i, NULLCOMMAND_i, NULLSTATUS_i} indicators_i; // indicators
typedef enum {CHANNEL_c, CURRENTMODE_c, STRAYMODE_c, RESET_c, DATARATE_c, SERVERUPDATE_c, ACK_c} cmdpackets_c; // command packets, list must match controller
typedef enum {CHANNELID_t, CURRENTHIGH_t, CURRENTLOW_t, CURRENTRATE_t, INDICATORS_t, INDICATORRATE_t, TEMPERATURE_t} telemetry_t; // telemetry types

class Node { // holds state of node
public:
  Node(); // default constructor
  void updateIndicators(); // referesh hardware indicators
  void setIndicatorValue(const indicators_i& itype, const byte& ival); // set indicator to specified value
  void setCurrentMeasurement(const unsigned int& measured); // from ADC
  void setTemperatureMeasurement(const byte& measured); // optional...
  void setChannelID(const byte& id); // set channel id
  void setCurrentRate(const byte& rate); // set sample rate
  void setIndicatorRate(const byte& rate); // set indicator rate
  void setLastCurrentTime(const unsigned long& last); // set last current sample time
  void setLastIndicatorTime(const unsigned long& last); // set last indicator sample time
  byte* getTelemetry() const; // return pointer to telemetry (byte-array)
  size_t getTelemetrySize() const; // returns size of telemetry
  byte getChannelID() const; // returns channel id
  byte getCurrentRate() const; // returns current rate
  byte getIndicatorRate() const; // returns indicator rate
  unsigned long getLastCurrentTime() const; // returns last current sample time
  unsigned long getLastIndicatorTime() const; // returns last indicator sample time
private:
  byte telemetry[7]; // stores indicator bits
  unsigned long lastCurrentTime; // time of last current transmission
  unsigned long lastIndicatorTime; // time of last indicator transmission
} data; // name of default object

Node::Node() { // default constructor
  for (int i = 0; i < sizeof(telemetry); i++) { telemetry[i] = NULL; } // set telemetry bytes to NULL (default)
}

void Node::updateIndicators() { // referesh hardware indicators
  data.setIndicatorValue(STRAY_i, digitalRead(1));
  data.setIndicatorValue(FIRED_i, digitalRead(2));
  data.setIndicatorValue(LOWCURRENTSTATUS_i, digitalRead(3));
  data.setIndicatorValue(NULLSTATUS_i, digitalRead(4));
}

void Node::setIndicatorValue(const indicators_i& itype, const byte& ival) { // sets bit corresponding to indicator type to a specified value
  if (ival == INDICATORS_t) telemetry[INDICATORS_t] &= (ival << itype);// set 0
  else telemetry[INDICATORS_t] |= (ival << itype); // set 1
}

void Node::setCurrentMeasurement(const unsigned int& measured) { // sets current by extracting bytes from 16-bit int
  telemetry[CURRENTHIGH_t] = highByte(measured);
  telemetry[CURRENTLOW_t] = lowByte(measured);
}

void Node::setTemperatureMeasurement(const byte& measured) { // sets temperature byte
  telemetry[TEMPERATURE_t] = measured;
}

void Node::setChannelID(const byte& id) { // set channel id
  telemetry[CHANNELID_t] = id;
}

void Node::setCurrentRate(const byte& rate) { // set current rate
  telemetry[CURRENTRATE_t] = rate;
}

void Node::setIndicatorRate(const byte& rate) { // set indicator rate
  telemetry[INDICATORRATE_t] = rate;
}

void Node::setLastCurrentTime(const unsigned long& last) { // set last current time
  lastCurrentTime = last;
}

void Node::setLastIndicatorTime(const unsigned long& last) { // set last indicator time
  lastIndicatorTime = last;
}

byte* Node::getTelemetry() const { // get pointer to first byte of telmetry array
  return telemetry;
}

size_t Node::getTelemetrySize() const { // get size of telemetry
  return sizeof(telemetry);
}

byte Node::getChannelID() const { // get channel id
  return telemetry[CHANNELID_t];
}

byte Node::getCurrentRate() const { // get current rate
  return telemetry[CURRENTRATE_t]; 
}

byte Node::getIndicatorRate() const { // get indicator rate
  return telemetry[INDICATORRATE_t]; 
}

unsigned long Node::getLastCurrentTime() const { // get last current sample time
  return lastCurrentTime;
}

unsigned long Node::getLastIndicatorTime() const { // get last indicators sample time
  return lastIndicatorTime;
}

void bufferCheck() { // check for new UDP payload
  int packetSize = Udp.parsePacket();
  
  if (packetSize) { // if there's data available, read a packet
    Serial.print("Received packet of size: ");
    Serial.println(packetSize);
    
    Serial.print("From: ");
    for (int i=0; i < 4; i++) {
      Serial.print(remoteIP[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    
    Serial.print(", Port: ");
    Serial.println(Udp.remotePort());

    Udp.read(rxBuffer, UDP_TX_PACKET_MAX_SIZE); // read the packet into packetBufffer
    Serial.print("Contents: ");
    for (int i = 0; i < UDP_TX_PACKET_MAX_SIZE; i++) { Serial.print(rxBuffer[i], HEX); } // print contents (in hex)
    Serial.println("");

    // check for command packets
    if (rxBuffer[0] == CHANNEL_c) {
      data.setChannelID(rxBuffer[1]); // update channel ID
    }
    else if (rxBuffer[0] == CURRENTMODE_c) {
      if (rxBuffer[1] == 0x00) { // 0 means LOW current mode commanded
        data.setIndicatorValue(HIGHCURRENTCOMMAND_i, 0); // set indicator to 0
        // send a digital out to hardware!!!
      }
      else if (rxBuffer[1] == 0x01) { // 1 means HIGH current mode commanded
        data.setIndicatorValue(HIGHCURRENTCOMMAND_i, 1); // set indicator to 1
        // send a digital out to hardware!!!
      }
    }
    else if (rxBuffer[0] == STRAYMODE_c) {
      // 0 for power-off (switch null resistor out of circuit)
      // 1 for power-on (switch null resistor into circuit)
      data.setIndicatorValue(NULLCOMMAND_i, rxBuffer[2]); // update null command
      // send a digital out to hardware!!!
    }
    else if (rxBuffer[0] == RESET_c) {
      data.setCurrentRate(NULL); // reset current rate to OFF
      // need to confirm the wording in SDR!!! interpreted as current measurements only... keep reporting indicators at low data rate???
    }
    else if (rxBuffer[0] == DATARATE_c) {
      data.setCurrentRate(rxBuffer[1]); // update current rate
      data.setIndicatorRate(rxBuffer[2]); // update indicator rate
    }

    // send a reply to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort()); // destination
    Udp.write(data.getChannelID()); // tx channel id
    Udp.write(ACK_c); // ack message
    Udp.write(rxBuffer[0]); // type of command ack'd
    Udp.endPacket();
  }
}

void setup() {
  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

  // start the Ethernet
  Ethernet.begin(macAddress); // dynamic IP

  Serial.begin(115200); // Open serial communications
  while (!Serial) {}; // wait for serial port to connect. Needed for native USB port only

  if (Ethernet.hardwareStatus() == EthernetNoHardware) { // Check for Ethernet hardware present
    Serial.println("Ethernet shield was not found.");
    while (true) {}; // do nothing, no point running without Ethernet hardware
  }
  
  if (Ethernet.linkStatus() == LinkOFF) Serial.println("Ethernet cable is not connected.");

  Udp.begin(localPort); // start UDP

  delay(1000); // give dhcp server time to respond

  Serial.println(Ethernet.localIP()); // print ip address

  for (int i = 0; i < sizeof(txBuffer); i++) { txBuffer[i] = macAddress[i]; } // load tx buffer with MAC

  while (data.getChannelID() == NULL) { // loop until channel acquired
    // send MAC to controller
    Udp.beginPacket(remoteIP, remotePort);
    Udp.write(CHANNEL_c); // request channel command
    Udp.write(macAddress, 6); // 6-byte MAC address
    Udp.endPacket();
    Serial.println("Initial contact sent!");
    
    delay(5000); // give server time to respond
    
    bufferCheck(); // check for response
  }
}

void sendSample() {
  unsigned long time = micros(); // update current time

  if (data.getIndicatorRate() && time - data.getLastIndicatorTime() > 1000000 / data.getIndicatorRate()) { // send indicator sample?
    data.updateIndicators(); // refresh indicators
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(data.getChannelID()); // tx channel id
    Udp.write(data.getTelemetry(), data.getTelemetrySize()); // send full telemetry
    Udp.endPacket();
    data.setLastIndicatorTime(time); // update time tx
    data.setLastCurrentTime(time); // current also sent with telemetry
  }
  else if (data.getCurrentRate() && time - data.getLastCurrentTime() > 1000000 / data.getCurrentRate()) { // send current sample?
    unsigned int currentMeasurement = analogRead(A0); // new sample from ADC
    data.setCurrentMeasurement(currentMeasurement); // store new sample
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(data.getTelemetry(), 3); // only send channel and current (the first 3 bytes of telemetry)
    Udp.endPacket();
    data.setLastCurrentTime(time); // update time tx
  }
}

void loop() {
  bufferCheck(); // check for new datagram
  sendSample(); // update and transmit new samples
  Serial.println("Loop");
  delay(10000);
  //time = micros() - time; // elapsed time
  //Serial.print("Packet rate: ");
  
  //Serial.println(2000*1000000/time); // packet rate in Hz
}
