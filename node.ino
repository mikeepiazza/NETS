/*
node.ino: collects data and reports over ethernet
*/

#include <Ethernet.h>
#include <EthernetUdp.h>

byte macAddress[] = {0x2C, 0xF7, 0xF1, 0x08, 0x24, 0x5C}; // MAC address

IPAddress remoteIP(192, 168, 0, 1); // controller IP address

unsigned int localPort = 8888; // node UDP port
unsigned int remotePort = 8888; // controller UDP port

// buffers for receiving and sending data
char rxBuffer[UDP_TX_PACKET_MAX_SIZE]; // receive buffer = 24B
char txBuffer[UDP_TX_PACKET_MAX_SIZE] = "initial"; // transmit buffer = 24B

// instance to let us send and receive packets over UDP
EthernetUDP Udp;

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

  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start UDP
  Udp.begin(localPort);

  delay(10);

  Serial.println(Ethernet.localIP());
  
  // contact controller
  Udp.beginPacket(remoteIP, remotePort);
  Udp.write(txBuffer);
  Udp.endPacket();
  Serial.println("initial sent!");
}

void loop() {
  int packetSize = Udp.parsePacket();
  
  if (packetSize) { // if there's data available, read a packet
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    
    Serial.print("From ");
    IPAddress remoteIP = Udp.remoteIP(); // update remote IP address
    for (int i=0; i < 4; i++) {
      Serial.print(remoteIP[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    Udp.read(rxBuffer, UDP_TX_PACKET_MAX_SIZE); // read the packet into packetBufffer
    Serial.println("Contents:");
    Serial.println(rxBuffer);

    // send a reply to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(txBuffer);
    Udp.endPacket();
  }
  delay(10);
}
