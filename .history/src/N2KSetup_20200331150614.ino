#include <ArduinoOTA.h>
#include <FastLED.h>
#include <WiFi.h>
//#include <WiFiUdp.h>

#define NUM_LEDS 2
#define DATA_PIN 0
CRGB leds[NUM_LEDS];

#define CANtx 23
#define CANrx 39
#define Vin 37
#define MOB 38

//#define peter 1
//#define ton 1
#define work 1
#ifdef peter
const char *ssid = "NicE_Engineering_UPC";
const char *password = "1001100110";
#endif
#ifdef ton
const char *ssid = "SSID";
const char *password = "Passw";
#endif
#ifdef work
const char *ssid = "airsupplies";
const char *password = "02351228133648477429";
#endif
const char *host = "N2K-bridge";

long TimeStamp;
int cnt = 0;
int threshold = 40;
bool touch1detected = false;
bool touch2detected = false;
bool touch3detected = false;

/*****************************************************************************/
/* Touch interrupt*/
/*****************************************************************************/
void gotTouch1() { touch1detected = true; }
void gotTouch2() { touch2detected = true; }
void gotTouch3() { touch3detected = true; }

/*****************************************************************************/
/* Over the air setup */
/* If timers are used kill the processes befor going to dwonload the new
 * firmware */
/* To realise that kill them in the function ArduinoOTA.onStart */
/**************************************************************************** */
void setup_OTA() {
  char buf[30];
  byte mac[6];
  Serial.println();
  Serial.print("[SETUP] OTA...");
  WiFi.macAddress(mac);
  sprintf(buf, "%s-%02x:%02x:%02x:%02x:%02x:%02x", host, mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);
  ArduinoOTA.setHostname(buf);
  ArduinoOTA.onStart([]() {
    /* switch off all processes here!!!!! */
    Serial.println();
    Serial.println("Recieving new firmware now!");
  });
  ArduinoOTA.onEnd([]() {
    /* do stuff after update here!! */
    Serial.println("\nRecieving done!");
    Serial.println("Storing in memory and reboot!");
    Serial.println();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });
  /* setup the OTA server */
  ArduinoOTA.begin();
  Serial.println("...done!");
}

/*****************************************************************************/
/*Setup*/
/*****************************************************************************/
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  pinMode(CANtx, OUTPUT);
  pinMode(CANrx, INPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  touchAttachInterrupt(T8, gotTouch1, threshold);
  touchAttachInterrupt(T7, gotTouch2, threshold);
  touchAttachInterrupt(T6, gotTouch3, threshold);
  void setup_OTA();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  TimeStamp = millis();
}

void loop() {
  ArduinoOTA.handle();
  if (TimeStamp + 1000 < millis()) {
    TimeStamp = millis();
    Serial.println((analogRead(Vin) * 3.3 / 4095) * 5.7);
    Serial.println(cnt++);
    if (leds[0].green > 0) {
      leds[0].green = 0;
    } else {
      leds[0].green = 25;
    }
    FastLED.show();
  }
  if (touch1detected) {
    touch1detected = false;
    Serial.println("Touch 1 detected");
    leds[1] = CRGB(0, 0, 25);
    FastLED.show();
  }
  if (touch2detected) {
    touch2detected = false;
    Serial.println("Touch 2 detected");
    leds[1] = CRGB(00, 25, 0);
    FastLED.show();
  }
  if (touch3detected) {
    touch3detected = false;
    Serial.println("Touch 3 detected");
    leds[1] = CRGB(25, 0, 0);
    FastLED.show();
  }
}
