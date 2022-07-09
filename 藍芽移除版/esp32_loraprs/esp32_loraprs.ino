#include <arduino-timer.h>
#include <DebugLog.h>
#include "WiFi.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* ssid = "3F";
const char* password = "9976192937";

#include "loraprs_service.h"

#if __has_include("/tmp/esp32_loraprs_config.h")
#pragma message("Using external config")
#include "/tmp/esp32_loraprs_config.h"
#else
#pragma message("Using default built-in config")
#include "config.h"
#endif

#if CFG_IS_CLIENT_MODE == true
#pragma message("Configured for client mode")
#else
#pragma message("Configured for server mode")
#endif

/*
 * Initialize config from config.h options.
 * Enables future use of EEPROM or allows user to dynamically modify config at run time.
 */
void initializeConfig(LoraPrs::Config &cfg) {

  // log level
  cfg.LogLevel = CFG_LOG_LEVEL;
  
  // client/server mode switch
  cfg.IsClientMode = CFG_IS_CLIENT_MODE;

  // lora parameters, must match on devices
  cfg.LoraFreqRx = CFG_LORA_FREQ_RX;
  cfg.LoraFreqTx = CFG_LORA_FREQ_TX;
  cfg.LoraBw = CFG_LORA_BW;
  cfg.LoraSf = CFG_LORA_SF;
  cfg.LoraCodingRate = CFG_LORA_CR;
  cfg.LoraSync = CFG_LORA_SYNC;
  cfg.LoraCrc = CFG_LORA_CRC; // set to 0 to disable
  cfg.LoraExplicit = CFG_LORA_EXPLICIT;
  cfg.LoraPower = CFG_LORA_PWR;

  // lora pinouts
  cfg.LoraPinSs = CFG_LORA_PIN_SS;
  cfg.LoraPinRst = CFG_LORA_PIN_RST;
  cfg.LoraPinA = CFG_LORA_PIN_A; // (sx127x - dio0, sx126x/sx128x - dio1)
  cfg.LoraPinB = CFG_LORA_PIN_B; // (sx127x - dio1, sx126x/sx128x - busy)
  cfg.LoraPinSwitchRx = CFG_LORA_PIN_RXEN;  // (sx127x - unused, sx126x - RXEN pin number)
  cfg.LoraPinSwitchTx = CFG_LORA_PIN_TXEN;  // (sx127x - unused, sx126x - TXEN pin number)
  cfg.LoraUseIsr = CFG_LORA_USE_ISR;  // set to true for incoming packet ISR usage (stream mode, e.g. speech)
  cfg.LoraUseCad = CFG_LORA_USE_CAD;  // carrier detect

  // aprs configuration
  cfg.AprsHost = "aprsc.tmmarc.org";
  cfg.AprsPort = 14580;
  cfg.AprsLogin = CFG_APRS_LOGIN;
  cfg.AprsPass = CFG_APRS_PASS;
  cfg.AprsFilter = CFG_APRS_FILTER; // multiple filters are space separated
  cfg.AprsRawBeacon = CFG_APRS_RAW_BKN;
  cfg.AprsRawBeaconPeriodMinutes = 10;

  // USB
  cfg.UsbSerialEnable = CFG_USB_SERIAL_ENABLE;

  // bluetooth device name
 // cfg.BtName = CFG_BT_NAME;
 // cfg.BtEnableBle = CFG_BT_USE_BLE;

  // server mode wifi paramaters
  cfg.WifiEnableAp = CFG_WIFI_ENABLE_AP;
  cfg.WifiSsid = CFG_WIFI_SSID;
  cfg.WifiKey = CFG_WIFI_KEY;

  // frequency correction
  cfg.EnableAutoFreqCorrection = CFG_FREQ_CORR;  // automatic tune to any incoming packet frequency
  cfg.AutoFreqCorrectionDeltaHz = CFG_FREQ_CORR_DELTA;

  // configuration flags and features
  cfg.EnableSignalReport = CFG_SIGNAL_REPORT;  // signal report will be added to the comment sent to aprsis
  cfg.EnablePersistentAprsConnection = CFG_PERSISTENT_APRS; // keep aprsis connection open, otherwise connect on new data only
  cfg.EnableRfToIs = CFG_RF_TO_IS;  // send data from rf to aprsis
  cfg.EnableIsToRf = CFG_IS_TO_RF; // send data from aprsis to rf
  cfg.EnableRepeater = CFG_DIGIREPEAT; // digirepeat incoming packets
  cfg.EnableBeacon = CFG_BEACON;  // enable periodic AprsRawBeacon beacon to rf and aprsis if rf to aprsis is enabled
  cfg.EnableTextPackets = CFG_TEXT_PACKETS; // enables text packets and disables KISS+AX25 binary frames for interoperability

  // kiss
  cfg.KissEnableExtensions = CFG_KISS_EXTENSIONS; // radio control and signal reports
  cfg.KissEnableTcpIp = CFG_KISS_TCP_IP;  // enable KISS ovr TCP/IP as a server

  // external ptt control
  cfg.PttEnable = CFG_PTT_ENABLE;
  cfg.PttPin = CFG_PTT_PIN;
  cfg.PttTxDelayMs = CFG_PTT_TX_DELAY_MS;
  cfg.PttTxTailMs = CFG_PTT_TX_TAIL_MS;
}

LoraPrs::Service loraPrsService;

auto watchdogLedTimer = timer_create_default();
AsyncWebServer server(80);
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, 1);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
  //Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial);
  
  LoraPrs::Config config;

  initializeConfig(config);
  loraPrsService.setup(config);

  watchdogLedTimer.every(LED_TOGGLE_PERIOD, toggleWatchdogLed);
}

void loop() {
  loraPrsService.loop();
  watchdogLedTimer.tick();
}

bool toggleWatchdogLed(void *) {
  digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
  return true;
}
