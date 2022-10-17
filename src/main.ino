#include "Arduino.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Adafruit_TSL2561_U.h"
#include "SPI.h"
#include "DHT.h"
#include "DHT_U.h"
#include "WiFi.h"
#include "Wire.h"
#include "TimeLib.h"

#define DHTONE      27
#define DHTTWO      14
#define I2C_SDA     33
#define I2C_SCK     32
#define I2C2_SDA    26
#define I2C2_SCK    25
#define RELAY_CH1   12
#define RELAY_CH2   10
#define TFT_SCK     18
#define TFT_MOSI    23
#define TFT_MISO    19
#define TFT_CS      22
#define TFT_DC      21
#define TFT_RESET   17

#define DHTTYPE     DHT22

DHT dhtone(DHTONE, DHTTYPE);
DHT dhttwo(DHTTWO, DHTTYPE);
Adafruit_TSL2561_Unified tslone = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_TSL2561_Unified tsltwo = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 11234);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RESET, TFT_MISO);

/* variables */
const char* ssid = "";
const char* pass = "";

float light1, light2, tempone, temptwo, avgLight, avgTemp;
int jam;

bool lampuIsOn = false;
bool nozzleIsOn = false;
int state = 0;

unsigned long nozzleTimer = 0;
unsigned long lastNozzleOnTimer = 0;
const unsigned long nozzleThresh = 60000;
const float lightThresh = 100;
const float tempThresh = 35;

time_t waktu;

void configureSensor(Adafruit_TSL2561_Unified tsli){
    tsli.enableAutoRange(true);
    tsli.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
}

float readTSL(Adafruit_TSL2561_Unified tsli){
    sensors_event_t event;
    tsli.getEvent(&event);
    return event.light;
}

void lightOn(){
    lampuIsOn = true;
    return;
}

void lightOff(){
    lampuIsOn = false;
    return;
}

void nozzleOn(){
    if(millis() - lastNozzleOnTimer < 2700000 && lastNozzleOnTimer != 0){
        return;
    }
    nozzleTimer = nozzleTimer == 0 ? millis() : nozzleTimer;
    lastNozzleOnTimer = 0;
    nozzleIsOn = millis() - nozzleTimer > 900000 ? false : true;
    return;
}

void nozzleOff(){
    if(nozzleTimer != 0){
        nozzleTimer = 0;
        lastNozzleOnTimer = millis();
    }
    nozzleIsOn = false;
    return;
}

void setup(){
    Serial.begin(115200);

    tft.begin();
    tft.fillScreen(ILI9341_WHITE);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);

    Wire.begin(I2C_SDA, I2C_SCK);
    Wire1.begin(I2C2_SDA, I2C2_SCK);

    Serial.println("connecting to tsl1");
    while(!tslone.begin(&Wire)){
        Serial.print(".");
        delay(100);
    }
    Serial.println("connected to tsl1");
    Serial.println("connecting to tsl2");
    while(!tsltwo.begin(&Wire1)){
        Serial.print(".");
        delay(100);
    }
    Serial.println("connected to tsl1");
    
    configureSensor(tslone);
    configureSensor(tsltwo);

    dhtone.begin();
    dhttwo.begin();

    Serial.print("\nconnecting to "); Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while(WiFi.status() != WL_CONNECTED){
        delay(1000);
        Serial.print(".");
    }
    Serial.print("\nconnected to "); Serial.println(ssid);

    configTime(0*3600, 0, "0.id.pool.ntp.org", "1.id.pool.ntp.org");
    delay(10000);

    Serial.println("");
}

void loop(){
    tempone = dhtone.readTemperature() - 3.2;
    temptwo = dhttwo.readTemperature() - 3.1;

    light1 = readTSL(tslone);
    light2 = readTSL(tsltwo);
    avgLight = (light1 + light2) / 2.0;
    avgTemp = (tempone + temptwo) / 2.0;

    if(isnan(tempone) || isnan(temptwo)){
        Serial.println(F("failed to read temperature from both sensor"));
    }

    waktu = time(nullptr);
    jam = hour(waktu) + 7 > 23 ? hour(waktu) + 7 -24 : hour(waktu) + 7;

    switch(state){
        case 0:
            nozzleOff();
            lightOff();
            break;
        case 1:
            nozzleOff();
            lightOn();
            break;
        case 2:
            nozzleOn();
            lightOff();
            break;
        case 3:
            nozzleOn();
            lightOn();
            break;
    }

    if((jam >= 6 && jam <= 19) && avgLight > lightThresh){
        if(avgTemp < tempThresh){
            state = 0;
        }
        else{
            state = 2;
        }
    }
    else{
        if(avgTemp < tempThresh){
            state = 1;
        }
        else{
            state = 3;
        }
    }

    Serial.print("jam     : "); Serial.println(jam);
    Serial.print("temp (1): "); Serial.print(tempone); Serial.println(" C");
    Serial.print("temp (2): "); Serial.print(temptwo); Serial.println(" C");
    Serial.print("tsl  (1): "); Serial.print(light1); Serial.println(" lux");
    Serial.print("tsl  (2): "); Serial.print(light2); Serial.println(" lux");
    Serial.println("--------");
    Serial.print("nozzle  : "); Serial.println(nozzleIsOn);
    Serial.print("lampu   : "); Serial.println(lampuIsOn);

    tft.setCursor(20, 20);
    tft.print("jam: "); tft.print(jam);
    tft.setCursor(20, 40);
    tft.print("temp (1): "); tft.print(tempone); tft.print(" C");
    tft.setCursor(20, 60);
    tft.print("temp (2): "); tft.print(temptwo); tft.print(" C"); 
    tft.setCursor(20, 80);
    tft.print("tsl (1): "); tft.print(light1); tft.print(" lux");
    tft.setCursor(20, 100);
    tft.print("tsl (2): "); tft.print(light2); tft.print(" lux");
    tft.setCursor(20, 120);
    tft.print("nozzle: "); tft.print(nozzleIsOn);
    tft.setCursor(20, 140);
    tft.print("lampu: "); tft.print(lampuIsOn);

    delay(1000);
}

