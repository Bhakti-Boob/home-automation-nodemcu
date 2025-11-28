/* Home Automation (NodeMCU)
  Functionality:
   - Connects ESP8266 to WiFi and SinricPro cloud 
   - Exposes multiple "Switch" devices in SinricPro (device_ID_1, device_ID_2, ...)
   - Maps each device ID to a physical relay pin and a local flip switch
   - Allows control from SinricPro (Alexa/Google/Home) and from local switches
   - Debounces local switches and sends state events back to SinricPro when user toggles local switches.
*/

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif 

#include <Arduino.h>
#include "SinricPro.h"         
#include "SinricProSwitch.h"   
#include <map>

// WiFi credentials 
#define WIFI_SSID "Bhakti"    
#define WIFI_PASS "xxxxxxxxx"

// SinricPro credentials 
#define APP_KEY "xxxxx-xxxxx-xxxxxxx-xxxxxxxxx-xxxx"
#define APP_SECRET "xxxxxx-xxxxxx-xxxxxx-xxxxxxx-xxxxxxx-xxxxxx-xxxxxx-xxxx"

// Device IDs created in SinricPro 
#define device_ID_1 "xxxxxxxxxxxxx"   // Light
#define device_ID_2 "xxxxxxxxxxxxx"   // Fan

// Relay pins driving relay modules
#define RelayPin1 5   // D1
#define RelayPin2 4   // D2

// Local switches  used for manual control
#define SwitchPin1 10  // SD3  
#define SwitchPin2 13  // D7   

// LED indicating WiFi state 
#define wifiLed 16   // D0

// Serial baud rate for debug output
#define BAUD_RATE 9600

// Debounce time for local switches (ms)
#define DEBOUNCE_TIME 250

// Hold mapping between a Sinric device and local pins
typedef struct {
  int relayPIN;       // physical relay control pin
  int flipSwitchPIN;  // local manual switch pin
} deviceConfig_t;

// Map of <SinricDeviceIdString, deviceConfig_t>
static std::map<String, deviceConfig_t> devices = {
  { device_ID_1, { RelayPin1, SwitchPin1 } },
  { device_ID_2, { RelayPin2, SwitchPin2 } },
};

// Hold runtime state for local switches to implement debounce & state tracking
typedef struct {
  String deviceId;                 
  bool lastFlipSwitchState;           // last read physical level from switch
  unsigned long lastFlipSwitchChange; // timestamp of last change 
} flipSwitchConfig_t;

// Map indexed by flipSwitch GPIO -> flipSwitchConfig_t
std::map<int, flipSwitchConfig_t> flipSwitches;

void setupRelays() {
  // Initialize all relay pins as outputs and set them to a safe "off" state
  for (auto &device : devices) {
    int relayPIN = device.second.relayPIN;
    pinMode(relayPIN, OUTPUT);
    digitalWrite(relayPIN, HIGH); // HIGH as initial state 
  }
}

void setupFlipSwitches() {
  // Build flipSwitches map: key = physical switch pin, value = deviceId & runtime state
  for (auto &device : devices)  {
    flipSwitchConfig_t flipSwitchConfig;
    flipSwitchConfig.deviceId = device.first;       // Sinric device id string
    flipSwitchConfig.lastFlipSwitchChange = 0;      // initialize debounce timestamp
    flipSwitchConfig.lastFlipSwitchState = true;    // assume HIGH (pull-up) as last state

    int flipSwitchPIN = device.second.flipSwitchPIN;
    flipSwitches[flipSwitchPIN] = flipSwitchConfig;

    // Configure pin as INPUT_PULLUP
    pinMode(flipSwitchPIN, INPUT_PULLUP);
  }
}

// Callback invoked when SinricPro (cloud) requests changing a switch power state
bool onPowerState(String deviceId, bool &state)
{
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");
  int relayPIN = devices[deviceId].relayPIN; // lookup relay pin from map
  digitalWrite(relayPIN, !state);            // set the relay 
  return true; 
}

// Polls all configured flip switches, debounces them and sends state events to SinricPro
void handleFlipSwitches() {
  unsigned long actualMillis = millis();

  // Iterate all known flip switches
  for (auto &flipSwitch : flipSwitches) {
    unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;

    // Only proceed if enough time has passed since last change (debounce)
    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {

      int flipSwitchPIN = flipSwitch.first; // the pin number (map key)
      bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;
      bool flipSwitchState = digitalRead(flipSwitchPIN); // read current (INPUT_PULLUP -> HIGH when not pressed)

      // If physical state changed since last read, handle it
      if (flipSwitchState != lastFlipSwitchState) {

#ifdef TACTILE_BUTTON
        // If TACTILE_BUTTON is defined, only react when the button is pressed 
        if (flipSwitchState) {
#endif
          flipSwitch.second.lastFlipSwitchChange = actualMillis; // update debounce timestamp
          String deviceId = flipSwitch.second.deviceId;          // which Sinric device to control
          int relayPIN = devices[deviceId].relayPIN;             // mapped relay pin
          bool newRelayState = !digitalRead(relayPIN);           // toggle current relay state
          digitalWrite(relayPIN, newRelayState);                 // set relay to new state
          // Notify SinricPro cloud of changed state so Google stays in sync
          SinricProSwitch &mySwitch = SinricPro[deviceId];
          mySwitch.sendPowerStateEvent(!newRelayState);

#ifdef TACTILE_BUTTON
        } // end tactile-press guard
#endif
        // update last flip switch state with newly read value
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;
      } 
    } 
  } 
}
// Network setup
void setupWiFi()
{
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  // Wait here until connected 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  // Turn wifi LED off 
  digitalWrite(wifiLed, LOW);
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}
void setupSinricPro()
{
  // Register callbacks for each device we declared in the 'devices' map
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState); // when cloud requests power state change -> call onPowerState
  }
  // Start the SinricPro client with APP_KEY and APP_SECRET
  SinricPro.begin(APP_KEY, APP_SECRET);
  // Restore device states stored locally or from SinricPro 
  SinricPro.restoreDeviceStates(true);
}
// Main loop
void setup()
{
  Serial.begin(BAUD_RATE);
  // wifi status LED pin, configured as OUTPUT
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, HIGH); // set LED to initial state 
  // initialize relays and switches and connect to network
  setupRelays();
  setupFlipSwitches();
  setupWiFi();
  setupSinricPro();
}
void loop()
{
  // SinricPro internal handling (receives cloud messages, keeps connection live)
  SinricPro.handle();
  // Poll local switches and handle manual toggles
  handleFlipSwitches();
}
