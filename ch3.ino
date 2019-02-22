// V1.0 TCP client that handles initial check-in
// V1.1 TCP server that receives commands from UI

#include <Ethernet.h>
#include <SPI.h>
#include <EthernetUDP.h>

byte mac[] = {0x12, 0x34, 0x56, 0x78, 0x91, 0x12}; // 6 byte mac address, used for network config, also change string version
String macStr = "12 34 56 78 91 12"; // string sent to server, compared to config.txt, must match byte version+
IPAddress ip(192, 168, 0, 4); // ip address object stored local static-ip
IPAddress server(192, 168, 0, 1); // server object stores server ip
int port = 13; // same port is used for all connections in NETS

byte buf[UDP_TX_PACKET_MAX_SIZE]; // declare send/recv buffer

byte channelID = 0; // assigned channel
byte groupID = 0; // assigned group

EthernetClient TCPclient; // declare TCP client object
EthernetServer TCPserver = EthernetServer(port); // declare TCP server object and assign port #
EthernetUDP UDPclient; // delcare UDP client object

void initialConfiguration() // receives/sets configuration parameters from server
{ 
  Serial.print("Initial configuration received: ");
  int numBytes = TCPclient.available(); // get number of bytes to read
  for (int i = 0; i < numBytes; i++)
  {
    buf[i] = TCPclient.read(); // read a byte into buffer
    Serial.print(buf[i], HEX);
  }
  Serial.println('\n');
  channelID = buf[0];
  groupID = buf[1];
}

void setup()
{
  Ethernet.begin(mac); // used to contact server before using static ip, could be superfluous with different server configuration
  Ethernet.begin(mac, ip); // use static ip
  
  Serial.begin(9600);
  Serial.println(Ethernet.localIP());

  while(channelID == 0) // while channelID is default
  {
    while(!TCPclient.connected()) // keep trying to connect until successful
    {
      Serial.print("Connecting to: ");
      Serial.println(server);
      
      TCPclient.connect(server, port); // connect to server ip on port 13
      delay(1000); // wait a second
    }

    Serial.println("Connected to server.\n");

    TCPclient.print(macStr); // send mac address to server
    delay(1000);
    initialConfiguration(); // handle response 
  }

  if (!TCPclient.connected()) // all data read from server?
  {
    Serial.println("Disconnecting.");
    TCPclient.stop(); // stop TCP client
  }

  // initial checkin complete, start tcp server to receive commands from ui
  TCPserver.begin(); // start listening for UI
  UDPclient.begin(port); // initialize UDP on port
}

void loop()
{
  EthernetClient ui = TCPserver.available(); // returns client object when a connection is accepted
  
  if (ui)
  { // client connected?
    Serial.print("ui detected");
    int numBytes = ui.available(); // get number of bytes to read
    for (int i = 0; i < numBytes; i++)
    {
      buf[i] = ui.read(); // read a byte into buffer
      Serial.print(buf[i], DEC);
    }
    Serial.println('\n');
    
    unsigned long time = micros(); // start time hack
    
    for (long i = 0; i < 200000; i++)
    {
      UDPclient.beginPacket(server, port);
      UDPclient.write(channelID);
      UDPclient.endPacket();
    }

    unsigned long elapsed = micros() - time;
    
    Serial.print("Elapsed time: ");
    Serial.println(elapsed);
    while(true);
  }
}
