/**
 * @file
 *
 * EspNowBroadcast.ino demonstrates how to perform ESP-NOW pseudo broadcast with @c WifiEspNowBroadcast .
 * You need two or more ESP8266 or ESP32 devices to run this example.
 *
 * All devices should run the same program.
 * You may need to modify the PIN numbers so that you can observe the effect.
 *
 * With the program running on several devices:
 * @li Press the button to transmit a message.
 * @li When a device receives a message, it will toggle its LED between "on" and "off" states.
 */

#include <WifiEspNowBroadcast.h>
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif

int prevValue = 1;
int buttonValue = 1;
int moving = 0;
int outPorts [] = {12, 27, 26, 25};


//Suggestion: the motor turns precisely when the ms range is between 3 and 20
void moveSteps(bool dir, int steps, byte ms) {
  for (unsigned long i = 0; i < steps; i++) {
    moveOneStep(dir); // Rotate a step
    delay(constrain(ms,3,20));        // Control the speed
  }
}

void moveOneStep(bool dir) {
  // Define a variable, use four low bit to indicate the state of port
  static byte out = 0x01;
  // Decide the shift direction according to the rotation direction
  if (dir) {  // ring shift left
    out != 0x08 ? out = out << 1 : out = 0x01;
  }
  else {      // ring shift right
    out != 0x01 ? out = out >> 1 : out = 0x08;
  }
  // Output singal to each port
  for (int i = 0; i < 4; i++) {
    digitalWrite(outPorts[i], (out & (0x01 << i)) ? HIGH : LOW);
  }
}

//start rotating motor if it's not currently rotating, stop rotating motor if it's currently rotating
void
processRx(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg)
{
  Serial.printf("Message from %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3],
                mac[4], mac[5]);
  for (size_t i = 0; i < count; ++i) {
    Serial.print(static_cast<char>(buf[i]));
  }
  Serial.println();

  //move motor
  if (moving == 0) {
    //start moving
    moveSteps(true, 64* 64, 3);
    delay(1000);
    moving = 1;
  }
  else {
    //stop moving
    moving = 0;
  }
}

void
setup()
{
  Serial.begin(115200);
  Serial.println();

  for (int i = 0; i < 4; i++) {
    pinMode(outPorts[i], OUTPUT);
  }


  WiFi.persistent(false);
  bool ok = WifiEspNowBroadcast.begin("ESPNOW", 70);
  Serial.println("working");
  if (!ok) {
    Serial.println("WifiEspNowBroadcast.begin() failed");
    ESP.restart();
  }
  // WifiEspNowBroadcast.begin() function sets WiFi to AP+STA mode.
  // The AP interface is also controlled by WifiEspNowBroadcast.
  // You may use the STA interface after calling WifiEspNowBroadcast.begin().
  // For best results, ensure all devices are using the same WiFi channel.

  WifiEspNowBroadcast.onReceive(processRx, nullptr);

  Serial.print("MAC address of this node is ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.println("Press the button to send a message");
}

void
sendMessage()
{
  char msg[60];
  int len = snprintf(msg, sizeof(msg), "hello ESP-NOW from %s at %lu",
                     WiFi.softAPmacAddress().c_str(), millis());
  WifiEspNowBroadcast.send(reinterpret_cast<const uint8_t*>(msg), len);

  Serial.println("Sending message");
  Serial.println(buttonValue);
  Serial.print("Recipients:");
  const int MAX_PEERS = 20;
  WifiEspNowPeerInfo peers[MAX_PEERS];
  
  int nPeers = std::min(WifiEspNow.listPeers(peers, MAX_PEERS), MAX_PEERS);
  
  for (int i = 0; i < nPeers; ++i) {
    Serial.printf(" %02X:%02X:%02X:%02X:%02X:%02X", peers[i].mac[0], peers[i].mac[1],
                  peers[i].mac[2], peers[i].mac[3], peers[i].mac[4], peers[i].mac[5]);
  }
}

void
loop()
{
  pinMode(2, INPUT_PULLUP);
  buttonValue = digitalRead(2);
  
  if (prevValue != buttonValue) { // button is pressed
    prevValue = buttonValue;
    sendMessage();
  }

  WifiEspNowBroadcast.loop();
  delay(10);
}
