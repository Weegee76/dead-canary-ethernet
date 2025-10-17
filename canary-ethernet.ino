// Dead Canary Sketch (Ethernet Edition)
// Inspired by rockettpunk on GitHub: https://github.com/rockettpunk/dead-canary
// Rewritten from the ground up for ESP32-based boards using a generic W5500 Ethernet module
// Serves a simple "Chirp" signal to a simple web portal, ping/curl to recieve the word "Chirp"
// Sends a Wake-on-LAN packet (magic packet) after a given delay when power is restored.

#include <SPI.h>          // required for SPI-based boards
#include <Ethernet.h>     // required for Ethernet
#include <EthernetUdp.h>  // required to Wake-on-LAN
#include <esp_mac.h>      // for using ESP32's MAC address
#include <WiFi.h>         // to disable wifi radios

// MODIFY FOR YOUR PURPOSE THIS SECTION ===========================================

// Network Configuration: Modify these values
IPAddress local_IP(0, 0, 0, 0);
IPAddress primaryDNS(0, 0, 0, 0);
IPAddress gateway(0, 0, 0, 0);
IPAddress subnet(0, 0, 0, 0);

// target MACs to wake: Add lines as needed and define MACs
const byte targetMac[][6] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

// Hardware Configuration: Modify these values
#define CS_PIN 8          // GPIO pin setting, connect to CS pin on W5500
#define DISABLE_RF true   // True to disable Wifi/Bluetooth, false to turn them on. Not needed for Ethernet
#define CPU_CLOCK 160     // CPU clock in MHz, to downclock and save power/thermals, comment to set to default
#define WAKE_TIME 600000  // Time to wait (in ms) before waking systems again, comment this line to disable WoL

// Nothing further requires modification ==========================================

EthernetServer server(80);  // Start server on port 80
uint8_t mac[6];             // Define Canary MAC address array
EthernetUDP Udp;
bool timer = true;

// Wake-on-LAN function
#if defined(WAKE_TIME)
void sendWol(const byte *macAddr) {
  Udp.beginPacket(IPAddress(255, 255, 255, 255), 9);  // broadcast, port 9

  byte packet[102];
  // 6×0xFF header
  memset(packet, 0xFF, 6);
  // 16×target MAC
  for (int i = 1; i <= 16; i++) {
    memcpy(&packet[i * 6], macAddr, 6);
  }

  Udp.write(packet, sizeof(packet));
  Udp.endPacket();

  Serial.print("Sent Wake-on-LAN packet to MAC: ");
  for (int i = 0; i < 6; i++) {
    if (i) Serial.print(':');
    if (macAddr[i] < 16) Serial.print('0');
    Serial.print(macAddr[i], HEX);
  }
  Serial.println();
}

// Wakes the rack array
void wakeRack() {
  for (size_t i = 0; i < sizeof(targetMac) / sizeof(targetMac[0]); i++) {
    sendWol(targetMac[i]);
    delay(1000);
  }
}
#endif

// Boot sequence
void setup() {
  Serial.begin(115200);   // Debug console
  Ethernet.init(CS_PIN);  // CS pin for W5500, user-defined

  // read the ESP32's burned‑in MAC, force unicast MAC
  esp_efuse_mac_get_default(mac);
  mac[0] |= 0x02;

  Serial.printf("Using MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Ethernet.begin(mac, local_IP, primaryDNS, gateway, subnet);
  Udp.begin(9);  // bind UDP port 9

  delay(1000);
  Serial.print("Initialized: IP: ");
  Serial.println(Ethernet.localIP());
  server.begin();

  // Disable wireless
  if (DISABLE_RF) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
  }

// Underclock CPU
#if defined(CPU_CLOCK)
  setCpuFrequencyMhz(CPU_CLOCK);
#endif
}

// Runtime
void loop() {
#if defined(WAKE_TIME)
  if (millis() >= WAKE_TIME && timer) {
    wakeRack();
    timer = false;
  }
#endif

  EthernetClient client = server.available();  // Open server
  if (!client) return;                         // Restart loop if no client connects

  // if client connects:
  // print request
  Serial.print("Request from ");
  Serial.println(client.remoteIP());

  // Response to client
  client.print(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "\r\n"  // end of headers
    "CHIRP\r\n");

  client.stop();  // Close client link, clear buffer
}
