#include <stdlib.h>
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_BME280.h"
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "EEPROM.h"

//#include "soc/soc.h"
//#include "soc/rtc_cntl_reg.h"
//void setup(){
//WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detecto

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

static int taskCore = 0;
String sensorsData = "";
bool enableLora = true;
bool enableWiFi = false;
bool enableEEPROM = true;
bool enableSensors = false;

unsigned long interval = 1000; // the time we need to wait
unsigned long previousMillis = 0;

// WIFI_LoRa_32 ports
// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)
//
// ---- LORA Pins ----

byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
int sw1Address = 0;
int sw2Address = 1;
int sw3Address = 2;
int sw4Address = 3;
int EEPROM_SIZE = 64;

#define BAND    433E6
#define SCK  5
#define MISO  19
#define MOSI  27
#define CS  18

#define SS  18
#define RST     14
#define DI0     26
// -----------------

String receivedText;
bool loraAvailable = false;
bool bmeAvailable = false;
bool eepromAvailable = false;
int wifiTryCount = 0;
int mqttTryCount = 0;
int wifiStatus = WL_IDLE_STATUS;

#define ORG "rqeofj"
#define BOARD_TYPE "HB_ESP32"
#define TOKEN "1SatnamW"

//#define HEARTBEAT_LED  32
bool hbLedState = false;

// #define DHTPin 35
#define ldrSensorPin 25

#define I2C_SCL 21
#define I2C_SDA 22
#define SEALEVELPRESSURE_HPA (1005)
#define BME280_ADD 0x76

#define TOUCH1PIN   4
#define TOUCH2PIN   2
#define TOUCH3PIN   13
#define TOUCH4PIN   15
unsigned long previousTouchMillis = 0;

int touchThreshold = 40;
int touchIndex = 0;

#define SW1  17
#define SW2  16
#define SW3  12
#define SW4  23

int sw1Val = 0;
int sw2Val = 0;
int sw3Val = 0;
int sw4Val = 0;

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char topic[] = "iot-2/evt/espboard/fmt/json";
char authMethod[] = "use-token-auth";
char token[] = "P@ssw0rd"; // Auth token of Device registered on Watson IoT Platform

String BOARD_ID;
WiFiClient wifiClient;
PubSubClient client(server, 1883, NULL, wifiClient);

const char* ssid = "GurvinderNet";
const char* password =  "1SatnamW";

// -----------------------------

int hallData = 0;
int boardTemp = 0;
float temperature = 0.0f;
float humidity = 0.0f;
float pressure = 0.0f;
float altitude = 0.0f;
float lightVal = 0.0f;

Adafruit_BME280 bme(I2C_SDA, I2C_SCL);

// --------------------

/**
 * Scan WiFi Networks
 */
static void scanWiFi(){
  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("Searching networks.");
  } else {
    Serial.println("Networks found: ");
    for (int i = 0; i < n; ++i) {
//      Print SSID for each network found
      char currentSSID[64];
      WiFi.SSID(i).toCharArray(currentSSID, 64);
      Serial.println(currentSSID);
    }
  }
  // Wait a bit before scanning again
  delay(5000);
}

/**
 * Connect to a given WiFi Network
 */
static void connectWiFi(){
  while(wifiStatus != WL_CONNECTED && wifiTryCount < 5){
        Serial.print("Attempting to connect to WEP network, SSID: ");
        Serial.println(ssid);
        wifiStatus = WiFi.begin(ssid, password);
        wifiTryCount++;
        delay(10000);
  }

  wifiTryCount = 0;

  if(wifiStatus == WL_CONNECTED){
    Serial.println("WiFi Connected..");
  }else{
    Serial.println("Failed to Connect WiFi");
  }
}

/**
 * Connect to MQTT Server
 */
static void connectMQTT() {
  // Serial.print("IN connecting MQTT client...");
  if(BOARD_ID == ""){
    char chipid[20];
    sprintf(chipid, "%" PRIu64, ESP.getEfuseMac());
    BOARD_ID = "HB_"+String(chipid);
  }
 String clientId = "d:" ORG ":" BOARD_TYPE ":" +BOARD_ID;
 if (!!!client.connected()) {
   Serial.print("Reconnecting MQTT client to ");
   Serial.println(server);
   while (!!!client.connect((char*) clientId.c_str(), authMethod, token)) {
     Serial.print("Connecting: ");
     Serial.println(BOARD_ID);
     delay(500);
   }
   // Serial.println();
 }
 Serial.print("MQTT Connected...");
}

/**
 * Get all sensors data and save it in a variable
 */
static void getSensorsData(){
    hallData = hallRead();
    boardTemp = (temprature_sens_read() - 32) / 1.8;
    lightVal = analogRead(ldrSensorPin);

    if(enableSensors){
        temperature = bme.readTemperature();
        humidity = bme.readHumidity();
        pressure = (bme.readPressure() / 100.0F);
        altitude = bme.readAltitude(SEALEVELPRESSURE_HPA); // Approx Altitude in meters
    }

/*
  if (isnan(temperature) || isnan(humidity)) {
    // Serial.println("Failed to read from BME280 sensor!");
    strcpy(celsiusTemp,"-0");
    strcpy(humidityTemp, "-0");
  } else{
    // Computes temperature values in Celsius + Fahrenheit and Humidity
    float hic = dht.computeHeatIndex(temperature, humidity, false);
    dtostrf(hic, 6, 2, celsiusTemp);
    dtostrf(humidity, 6, 2, humidityTemp);
  }
*/
//  sensorsData = "{\"type\":\"" BOARD_TYPE "\", \"uniqueId\":\"" +BOARD_ID+ "\", \"BOARD_TEMP\":" +String(boardTemp)+", \"HALL_DATA\":" +String(hallData)+", \"TEMP\":" +String(temperature)+", \"HUM\":" +String(humidity)+", \"PRESSURE\":" +String(pressure)+", \"ALT\":" +String(altitude)+", \"LIGHT\":" +String(lightVal)+"}";
  sensorsData = "{\"type\":\"" BOARD_TYPE "\", \"uniqueId\":\"" +BOARD_ID+ "\", \"d\": {\"type\":\"" BOARD_TYPE "\", \"uniqueId\":\"" +BOARD_ID+ "\", \"BOARD_TEMP\":" +String(boardTemp)+", \"HALL_DATA\":" +String(hallData)+", \"TEMP\":" +String(temperature)+", \"HUM\":" +String(humidity)+", \"PRESSURE\":" +String(pressure)+", \"ALT\":" +String(altitude)+", \"LIGHT\":" +String(lightVal)+"}}";

}

void gotTouch1(){
  touchIndex = 1;
}

void gotTouch2(){
  touchIndex = 2;
}

void gotTouch3(){
  touchIndex = 3;
}

void gotTouch4(){
  touchIndex = 4;
}

void publishData(String data){


  Serial.print("Publish data:>> ");
  Serial.println(data);
     /*
   if (client.publish(topic, (char*) payload.c_str())) {
       // Serial.print("Published payload: ");
       // Serial.println(payload);
     } else {
       // Serial.println("Publish failed: ");
       // Serial.println(payload);
     }
  */

  if(loraAvailable){
        LoRa.beginPacket();
//        LoRa.write(destination);              // add destination address
//        LoRa.write(localAddress);
        LoRa.print(data);
        LoRa.print("\n");
        LoRa.endPacket();
//        LoRa.receive();
        delay(1);
        LoRa.flush();
    }else{
       Serial.print("Lora Not Working: >> ");
       Serial.println(data);
    }
}

// tocuhValue > threshold = Release Event
// touchValue < threshold = Touch Event
void checkTouchDetected(){
  if(touchIndex > 0){
    unsigned long currentTouchMillis = millis();
    if ((unsigned long)(currentTouchMillis - previousTouchMillis) >= 600) {
          previousTouchMillis = millis();
//          printf(touchIndex);
          switch (touchIndex){
            case 1: {
                int touchVal = touchRead(TOUCH1PIN);
                if(touchVal > 1 && touchVal <= touchThreshold){
                      if(sw1Val == 1){
                        sw1Val = 0;
                      }else{
                        sw1Val = 1;
                      }
                      digitalWrite(SW1, sw1Val);
                      String payload = "{\"type\":\"" BOARD_TYPE "\", \"uniqueId\":\"" +BOARD_ID+"\", \"deviceIndex\":1, \"deviceValue\": " +String(sw1Val)+"}";
                      publishData(payload);
                      if(eepromAvailable){
                          EEPROM.writeInt(sw1Address, sw1Val);
                          // Serial.printf("sw1Val: %d \n", EEPROM.readInt(sw1Address));
                      }
                }
                break;
            }
           case 2: {
              int touchVal = touchRead(TOUCH2PIN);
              if(touchVal > 1 && touchVal <= touchThreshold){
                    if(sw2Val == 1){
                      sw2Val = 0;
                    }else{
                      sw2Val = 1;
                    }
                    digitalWrite(SW2, sw2Val);
                    String payload = "{\"type\":\"" BOARD_TYPE "\", \"uniqueId\":\"" +BOARD_ID+"\", \"deviceIndex\":2, \"deviceValue\": " +String(sw2Val)+"}";
                    publishData(payload);
                    if(eepromAvailable){
                      EEPROM.writeInt(sw2Address, sw2Val);
                      // Serial.printf("sw2Val: %d \n", EEPROM.readInt(sw2Address));
                    }
              }
              break;
           }
           case 3: {
              int touchVal = touchRead(TOUCH3PIN);
              if(touchVal > 1 && touchVal <= touchThreshold){
                    if(sw3Val == 1){
                      sw3Val = 0;
                    }else{
                      sw3Val = 1;
                    }
                    digitalWrite(SW3, HIGH);
                    String payload = "{\"type\":\"" BOARD_TYPE "\", \"uniqueId\":\"" +BOARD_ID+"\", \"deviceIndex\":3, \"deviceValue\": " +String(sw3Val)+"}";
                    publishData(payload);
                    if(eepromAvailable){
                      EEPROM.writeInt(sw3Address, sw3Val);
                      // Serial.printf("sw3Val: %d \n", EEPROM.readInt(sw3Address));
                    }
              }
              break;
           }
           case 4: {
              int touchVal = touchRead(TOUCH4PIN);
              if(touchVal > 1 && touchVal <= touchThreshold){
                    if(sw4Val == 1){
                      sw4Val = 0;
                    }else{
                      sw4Val = 1;
                    }
                    digitalWrite(SW4, sw4Val);
                    String payload = "{\"type\":\"" BOARD_TYPE "\", \"uniqueId\":\"" +BOARD_ID+"\", \"deviceIndex\":4, \"deviceValue\": " +String(sw4Val)+"}";
                    publishData(payload);
                    if(eepromAvailable){
                      EEPROM.writeInt(sw4Address, sw4Val);
                      // Serial.printf("sw4Val: %d \n", EEPROM.readInt(sw4Address));
                    }
              }
              break;
           }
          }
          // delay(1);
          if(eepromAvailable){
            // EEPROM.commit();
          }
          touchIndex = 0;
//          delay(1);
     }
  }

}

void checkDataOnLora(){
    // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    // Serial.print("Received packet '");
    // read packet
    while (LoRa.available()) {
      receivedText = (char)LoRa.read();
      Serial.print(receivedText);
    }

    // print RSSI of packet
    // Serial.print("' with RSSI ");
    // Serial.println(LoRa.packetRssi());
  }
}

void initSwitches(){
  if(eepromAvailable){
    sw1Val = EEPROM.readInt(sw1Address);
    sw2Val = EEPROM.readInt(sw2Address);
    sw3Val = EEPROM.readInt(sw3Address);
    sw4Val = EEPROM.readInt(sw4Address);
  }

  if(sw1Val < 0){
    sw1Val = 0;
  }
  if(sw2Val < 0){
    sw2Val = 0;
  }
  if(sw3Val < 0){
    sw3Val = 0;
  }
  if(sw4Val < 0){
    sw4Val = 0;
  }

  if(sw1Val > 1){
    sw1Val = 1;
  }
  if(sw2Val > 1){
    sw2Val = 1;
  }
  if(sw3Val > 1){
    sw3Val = 1;
  }
  if(sw4Val > 1){
    sw4Val = 1;
  }

    digitalWrite (SW1, sw1Val);
    digitalWrite (SW2, sw2Val);
    digitalWrite (SW3, sw3Val);
    digitalWrite (SW4, sw4Val);
    // Serial.printf("Switch 1: %d, Switch 2: %d, Switch 3: %d, Switch 4: %d\n\n", sw1Val, sw2Val, sw3Val, sw4Val);

}

// the setup function runs once when you press reset or power the board
void setup() {
  delay(1000);

//  pinMode(HEARTBEAT_LED, OUTPUT);
  pinMode(SW1, OUTPUT);
  pinMode(SW2, OUTPUT);
  pinMode(SW3, OUTPUT);
  pinMode(SW4, OUTPUT);

  if(enableSensors){
        // Init BME280 Sensor
        int bmeTryCount = 0;
        do{
          bmeAvailable = bme.begin(BME280_ADD);
          bmeTryCount++;
          if(!bmeAvailable){
            // Serial.printf("Could not find a valid BME280 sensor, check wiring!, Try Count: %d\n", bmeTryCount);
            delay(2000);
          }
        } while(!bmeAvailable && bmeTryCount < 2);
  }

  // Init Lora
  if(enableLora){
      SPI.begin(SCK, MISO, MOSI, CS);
      LoRa.setPins(SS, RST, DI0);
      delay(1000);
  }
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  char chipid[20];
  sprintf(chipid, "%" PRIu64, ESP.getEfuseMac());
  BOARD_ID = "HB_"+String(chipid);
  // Serial.println(BOARD_ID);
  delay(100);

if(enableEEPROM){
    int eepromTryCount = 0;
    do{
      eepromAvailable = EEPROM.begin(EEPROM_SIZE);
      eepromTryCount++;
      if(!eepromAvailable){
        Serial.printf("Failed to initialise EEPROM, Try Count: %d\n", eepromTryCount);
        delay(2000);
      }else{
        Serial.println("EEPROM Initialized Successfully...");
      }
    }while(!eepromAvailable && eepromTryCount > 3);
}

  // Serial.println("LoRa Initializing...");
  if(enableLora){
      int loraTryCount = 0;
      do{
        loraAvailable = LoRa.begin(BAND);
        loraTryCount++;
        if(!loraAvailable){
          Serial.printf("Starting LoRa failed!, Try Count: %d\n", loraTryCount);
          delay(3000);
        }else{
          Serial.println("LoRa Initialized Successfully...");
        }
      }while(!loraAvailable && loraTryCount < 3);
  }


if(enableWiFi){
      // Set WiFi to station mode and disconnect from an AP if it was previously connected
//      WiFi.mode(WIFI_STA);
//      WiFi.disconnect();
      delay(100);

//      scanWiFi();
      connectWiFi();
       if(wifiStatus == WL_CONNECTED){
          connectMQTT();
       }
}

    touchAttachInterrupt(TOUCH1PIN, gotTouch1, touchThreshold);
    touchAttachInterrupt(TOUCH2PIN, gotTouch2, touchThreshold);
    touchAttachInterrupt(TOUCH3PIN, gotTouch3, touchThreshold);
    touchAttachInterrupt(TOUCH4PIN, gotTouch4, touchThreshold);

    initSwitches();

}

  // the loop function runs over and over again forever
  void loop() {
    unsigned long currentMillis = millis();
      checkDataOnLora();
      checkTouchDetected();
      if ((unsigned long)(currentMillis - previousMillis) >= (interval * 15)) {
           hbLedState = !hbLedState;
           if(hbLedState){
//             digitalWrite(HEARTBEAT_LED, 1);
               Serial.println("...");
           }else{
//             digitalWrite(HEARTBEAT_LED, 0);
               Serial.println(BOARD_ID);
           }
           getSensorsData();
           publishData(sensorsData);
           previousMillis =  millis();
      }

      //  scanWiFi();
      // Serial.println(BOARD_ID);

  }
