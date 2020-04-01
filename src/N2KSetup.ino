#include <WiFi.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include <driver/rtc_io.h>

/*Defines*/
#define NUM_LEDS 2
#define WS2812_PIN 4
#define CANtx 2
#define CANrx 15
#define Vin 37
#define MOB 38
#define NMEA38400 5
#define NMEA4800 18
#define NMEAout 23
#define Relais 0
#define peter 1
//#define ton 1
//#define work 1
/*End difines*/

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
IPAddress ipLok;
bool WiFi_ok = false;
CRGB leds[NUM_LEDS];

long TimeStamp;
int cnt = 0;
float V12;
int threshold = 40;
bool touch1detected = false;
bool touch2detected = false;
bool touch3detected = false;
bool MOBactive = false;

/*****************************************************************************/
/* Touch interrupt*/
/*****************************************************************************/
void gotTouch1() { touch1detected = true; }
void gotTouch2() { touch2detected = true; }
void gotTouch3() { touch3detected = true; }
/*****************************************************************************/
/* BOB interrupt*/
/*****************************************************************************/
void IRAM_ATTR MOBtsk() { MOBactive = true; }

/*****************************************************************************/
/* Over the air setup */
/* If timers are used kill the processes befor going to dwonload the new
 * firmware */
/* To realise that kill them in the function ArduinoOTA.onStart */
/**************************************************************************** */
void setup_OTA()
{
    char buf[30];
    byte mac[6];
    Serial.println();
    Serial.print("[SETUP] OTA...");
    WiFi.macAddress(mac);
    sprintf(buf, "%s-%02x:%02x:%02x:%02x:%02x:%02x", host, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print(buf);
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
void setup()
{
    Serial.begin(115200);
    Serial.println("[SETUP] Booting");

    pinMode(CANtx, OUTPUT);
    pinMode(CANrx, INPUT_PULLUP);
    pinMode(MOB, INPUT_PULLUP);
    pinMode(NMEA38400, INPUT_PULLUP);
    pinMode(NMEA4800, INPUT_PULLUP);
    pinMode(NMEAout, OUTPUT);
    pinMode(Relais, OUTPUT);
    FastLED.addLeds<NEOPIXEL, WS2812_PIN>(leds, NUM_LEDS);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    TimeStamp = millis();
    Serial.print("[SETUP] ");
    while (!WiFi_ok && TimeStamp + 5000 > millis())
    {
        WiFi_ok = !(WiFi.waitForConnectResult() != WL_CONNECTED);
        delay(100);
        Serial.print("*");
        if (leds[0].blue > 0)
        {
            leds[0].blue = 0;
            leds[1].blue = 5;
        }
        else
        {
            leds[0].blue = 5;
            leds[1].blue = 0;
        }
        FastLED.show();
    }
    leds[0].blue = 0;
    leds[1].blue = 0;
    FastLED.show();

    if (WiFi_ok)
    {
        Serial.println("\r\n[SETUP] Network found and logged in");
        ipLok = WiFi.localIP();
    }
    else
    {
        Serial.println("\r\n[SETUP] Setup AP");
        WiFi.softAP(ssid, password);
        ipLok = WiFi.softAPIP();
    }
    Serial.print("[SETUP] IP address: ");
    Serial.print(ipLok);
    touchAttachInterrupt(T8, gotTouch1, threshold); // depends on version sdk use T8 or T9
    touchAttachInterrupt(T7, gotTouch2, threshold);
    touchAttachInterrupt(T6, gotTouch3, threshold);
    attachInterrupt(MOB, MOBtsk, FALLING); // init MOB interrupt
    setup_OTA();
    Serial.println("[SETUP] Ready");
    TimeStamp = millis();
    touch1detected = false; // set touch to fase after start up
    touch2detected = false;
    touch3detected = false;
}

void loop()
{
    ArduinoOTA.handle();
    if (TimeStamp + 500 < millis())
    {
        TimeStamp = millis();
        V12 = analogRead(Vin) * 3.6 * 5.7 / 4095;
        Serial.printf("\rVoltage: %02.1f NMEA38400: %d NMEA4800: %d CANrx: %d", V12, digitalRead(NMEA38400), digitalRead(NMEA4800), digitalRead(CANrx));
        if (!digitalRead(Relais))
        {
            digitalWrite(CANtx, true);
            digitalWrite(NMEAout, true);
            digitalWrite(Relais, true);
        }
        else
        {
            digitalWrite(CANtx, false);
            digitalWrite(NMEAout, false);
            digitalWrite(Relais, false);
        }
        if (cnt)
        {
            cnt--;
        }
        else
        {
            leds[1].red = 0;
            leds[1].green = 0;
            leds[1].blue = 0;
            leds[0].red = 0;
            leds[0].green = 0;
            leds[0].blue = 0;
        }
        FastLED.show();
    }

    if (MOBactive == true)
    {
        delay(100); //debounce
        if (!digitalRead(MOB))
        {
            cnt = 3;
            leds[0].red = 255;
            FastLED.show();
            Serial.println("MOB pushed!!!");
        }
        MOBactive = false;
    }

    if (touch1detected || touch2detected || touch3detected)
    {
        if (touch1detected)
        {
            touch1detected = false;
            Serial.println("Touch 1 detected");
            leds[1].blue = 5;
        }
        if (touch2detected)
        {
            touch2detected = false;
            Serial.println("Touch 2 detected");
            leds[1].green = 5;
        }
        if (touch3detected)
        {
            touch3detected = false;
            Serial.println("Touch 3 detected");
            leds[1].red = 5;
        }
        FastLED.show();
        cnt = 5;
    }
}