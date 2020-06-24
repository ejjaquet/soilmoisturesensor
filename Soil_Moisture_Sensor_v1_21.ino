//v1.1 - Max time set to 15m
//v1.2.1 - Copied from file on thingiverse

//WiFi enabled Soil Moisture Sesnor v2
//Copyright cabuu, 2018
//For more details see http://www.cabuu.com


//Configure the following 3 variables

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).

char auth[] = "aIKKJm6H0WhWOQHYYOFgbIZuKymi3Ygw";

//End of configuration

//needed for wifi manager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "FastLED.h"
#include <EEPROM.h>

//Fast LED definitions
#define NUM_LEDS      1
#define DATA_PIN      4
#define COLOR_ORDER   GRB
#define CHIPSET       WS2812B
#define FREQUENCY     50                // controls the interval between strikes
#define FLASHES       8                 // the upper limit of flashes per strike
#define BRIGHTNESS    10

int val;
int moistureThreshold;                  //Soil moisture level
int soilMoistureLevel;
float soilMoistureLevelf;               //In float form
int Status = 1;                         //Status for LED etc, 1 = Green (OK), 2 = Red (Dry), 3=Grey (Offline), 4 = Blue (Too wet)
int ledStatus = 1;                      //Led On (Default)

int addrMoistureThresholdUpper = 0;     //Memory index of upper moisture threshold
float moistureThresholdUpper;

int addrMoistureThresholdLower = 10;    //Memory index of lower moisture threshold
float moistureThresholdLower;


int addrSleepMode = 50;                 //Memory index of sleep mode inidcator
int SleepMode;                          //Sleep mode, default 1 = on

//Set dry and wet levels based on your own sensor, in my case Dry 28, wet =69
int dryPercent = 28;
int wetPercent = 69;


BlynkTimer timer;

CRGB leds[NUM_LEDS];
#define color White;
unsigned int dimmer = 1;


BLYNK_WRITE(V16)
{
  SleepMode = param.asInt();
  Serial.println("Sleep Mode Set To: ");
  Serial.println(SleepMode);
  EEPROM.begin(512);
  EEPROM.put(addrSleepMode, SleepMode);
  Serial.println("Saving...");
  EEPROM.commit(); 
}

BLYNK_WRITE(V4)
{
  //Turn on/off LED
  int pinValue = param.asInt(); // assigning incoming value from pin V4 to a variable
  if (pinValue==1) {
    ledStatus = 1;
  } else {
    //Turn off the LED
        leds[0] = CRGB::Black; 
        FastLED.show(); 
        ledStatus = 0;
  }
  
}

BLYNK_WRITE(V8)
{
  //Executes on V8 change in app
  int pinValue = param.asInt(); // assigning incoming value from pin V8 to a variable
  //Convert to float
  moistureThresholdUpper = (float)pinValue;
  
  //Update EEPROM now
  uint addr = 0;
  EEPROM.begin(512);
  EEPROM.put(addrMoistureThresholdUpper, moistureThresholdUpper);
  Serial.println("Saving...");
  EEPROM.commit(); 
  moistureThresholdUpper = 0;
  EEPROM.get(addrMoistureThresholdUpper, moistureThresholdUpper);
  Serial.println(moistureThresholdUpper);
}

BLYNK_WRITE(V9)
{
  //Executes on V8 change in app
  int pinValue = param.asInt(); // assigning incoming value from pin V8 to a variable
  //Convert to float
  moistureThresholdLower = (float)pinValue;
  
  //Update EEPROM now
  uint addr = 0;
  EEPROM.begin(512);
  EEPROM.put(addrMoistureThresholdLower, moistureThresholdLower);
  Serial.println("Saving...");
  EEPROM.commit(); 
  moistureThresholdLower = 0;
  EEPROM.get(addrMoistureThresholdLower, moistureThresholdLower);
  Serial.println(moistureThresholdLower);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  Serial.begin(9600);                     // Debug console
  Serial.println("Soil Moisture Sensor v1.2");     // Soil moisture sensor v1
  delay( 3000 );                          // power-up safety delay

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect("MoistureSensor01")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);

  } 

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  Blynk.config(auth);

  if(!Blynk.connected()) {
    Blynk.connect();
  }


  EEPROM.begin(512);

  EEPROM.get(addrMoistureThresholdUpper,moistureThresholdUpper);  //Get the upper moisture threshold
  EEPROM.get(addrMoistureThresholdLower,moistureThresholdLower);  //Get the upper moisture threshold
  EEPROM.get(addrSleepMode,SleepMode);     // Get sleep status 
  Serial.println("bef blynk");
  // Blynk.begin(auth, ssid, pass);          // Connect to Blynk
  Serial.println("after blynk");
  timer.setInterval(1000L, myTimerEvent);
  Serial.println("after timer");
  //Sync with values saved since last sleep, in case we have asked for it to wake up
  Serial.println("Syncing with Blynk...");
  delay(500);
  Blynk.syncAll();
  delay(500);
  Serial.println("Done!");
    
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  LEDS.setBrightness(BRIGHTNESS);
  // put your setup code here, to run once:

  // Flash green to inidciate sucesful startup
        leds[0] = CRGB::Green; 
        FastLED.show(); 
        delay(100);
        leds[0] = CRGB::Black; 
        FastLED.show(); 
        delay(100);
        leds[0] = CRGB::Green; 
        FastLED.show(); 
        delay(100);
        leds[0] = CRGB::Black; 
        FastLED.show(); 


        WidgetLED StatusLED(V11);
        StatusLED.on();

        WidgetLED MoistureLED(V12);
        MoistureLED.on();

        //Set App status LED to Green to show we are connected
        Blynk.setProperty(V11, "color","#4ca64c");      // Set staus led in app to green
}

void myTimerEvent()
{
  delay(100);
Serial.println("checking...");
  // Check the moisture level
  GetMoistureLevel();
  delay(100);

  Serial.println(soilMoistureLevel);
    Serial.println("/");
  Serial.println(moistureThresholdLower);
  
  if(soilMoistureLevel <int(moistureThresholdUpper) && soilMoistureLevel >int(moistureThresholdLower)) //Soil is perfect
  {
        if (ledStatus == 1){
          leds[0] = CRGB::Green; 
          FastLED.show(); 
        }


        Blynk.setProperty(V12, "color","#4ca64c");      // Set staus led in app to green
  }
  
  else if(soilMoistureLevel >int(moistureThresholdUpper)) //Soil is wet
  {
        if (ledStatus == 1){
          leds[0] = CRGB::Blue; 
          FastLED.show(); 
        }
        Blynk.setProperty(V12, "color","#6666ff");      // Set staus led in app to blue
        
        Blynk.notify("Moisture High");
  }
  else if(soilMoistureLevel <int(moistureThresholdLower)) //Soil is wet
  {
        if (ledStatus == 1){
          leds[0] = CRGB::Red; 
          FastLED.show(); 
        }
        Blynk.setProperty(V12, "color","#ff3232");      // Set staus led in app to red
        Blynk.notify("Moisture Low");
        
  }

  //Update Blynk app
  Blynk.virtualWrite(V5, soilMoistureLevel);
}


void loop() {
  // put your main code here, to run repeatedly:
  Blynk.run();
  timer.run(); // Initiates BlynkTimer

if (SleepMode ==1) {
  //Done everything now Gotosleep
  //If LED state currently Green i.e Status = 1, set staus LED to Grey (Offline)
  //if (Status == 1) {
  delay(500);
  FastLED.setBrightness(1);
  Blynk.setProperty(V11, "color","#808080");      // Set staus led in app to grey
  delay(500);
  //}
  Serial.println("Sleeping...");
  ESP.deepSleep(15 * 60 * 1000000); // deepSleep time is defined in microseconds.
} else {
  //Do nothing
  delay(1000);
  Serial.println("Staying Awake...");
}

  

}


//Set dry and wet levels based on your own sensor, in my case Dry 28, wet =69
    void GetMoistureLevel()
  {
    delay(100);
    soilMoistureLevelf = analogRead(0);
    soilMoistureLevelf = (1-(soilMoistureLevelf/1023))*100;
    //Convert to a meaningful percentage based on our calibrated wet/dry levels defined earlier
    soilMoistureLevelf = ((soilMoistureLevelf - dryPercent)/(wetPercent-dryPercent))*100;
    soilMoistureLevel = int(soilMoistureLevelf);
    delay(100);
  }

  BLYNK_CONNECTED() {
  }
