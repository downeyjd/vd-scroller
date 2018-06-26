#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/*extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}*/

//adafruit.io credentials
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME     "downeyjd"
#define AIO_KEY          "5eb2369140e04b4abaa5bfd270718841"
#define HEART_TOPIC      "downeyjd/feeds/heart"
#define BATT_TOPIC      "downeyjd/feeds/batt"
#define HEART_PING      "downeyjd/feeds/heart/get"
#define DONE_PIN        12

ADC_MODE(ADC_VCC); //setup ESP8266 ADC to measure VCC and NOT use A0 pin
//uint16_t current = 0;
//uint16_t last = 0;
char* wifiaps[2][2] = {{"bleepblorp", "1234"}, {"blerg", "VUrocks00!!"}};
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish batt = Adafruit_MQTT_Publish(&mqtt, BATT_TOPIC);
Adafruit_MQTT_Subscribe heart = Adafruit_MQTT_Subscribe(&mqtt, HEART_TOPIC);
Adafruit_MQTT_Publish heartClear = Adafruit_MQTT_Publish(&mqtt, HEART_TOPIC);

//setup charliplexed LED controller board
Adafruit_IS31FL3731 matrix = Adafruit_IS31FL3731();

static const uint8_t PROGMEM
  hearts[] = 
  { 0x3E, 0x51, 0xDC, 0x08, 0x20,
    0x08, 0xFB, 0xFE, 0x08, 0x20,
    0x08, 0x73, 0xFE, 0x08, 0x20,
    0x08, 0x21, 0xFC, 0x08, 0x20,
    0x08, 0x00, 0xF9, 0x48, 0x20,
    0x08, 0x00, 0x73, 0xE8, 0x20,
    0x08, 0x00, 0x21, 0xC8, 0x20,
    0x3E, 0x00, 0x00, 0x87, 0xC0 };

/*typedef struct {
  uint16_t goodwifiap;
  uint16_t lastbatt;
} RTC_MEM;

RTC_MEM rtc_test;
*/

void drawhearts() {
  matrix.setRotation(0);
  matrix.clear();
  for (int8_t x=36; x>=-36; x--) {
    matrix.clear();
    matrix.drawBitmap(x,0, hearts, 36, 8, 255/4);
    delay(25);
  }  
}

bool wifi_connect() {
  // connect to WiFi/AIO
  /*Serial.println();
  Serial.print("Connecting to last known WiFi AP..");
  WiFi.begin(wifiaps[rtc_test.goodwifiap][0], wifiaps[rtc_test.goodwifiap][1]); //test last known "good" WiFi AP
  uint8_t counter = 0;
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    counter++;
    if(counter > 10) {
      break; 
    }
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println(" Success!");
    return true;
  }*/
  while(WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("Trying all known APs. ");
    for(int i=0; i < sizeof(wifiaps)/sizeof(*wifiaps); i++) {
      Serial.print(wifiaps[i][0]);
      WiFi.begin(wifiaps[i][0], wifiaps[i][1]);
      uint8_t counter = 0;
      while(WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
        counter++;
        if(counter > 15) {
          break; 
        }
      }
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println(" Success!");
        return true;
      }
      Serial.println(" Failed!");    
    }
    break;
  }
  return false;
}

void setup() {
  
  pinMode(DONE_PIN, OUTPUT);
  // start the serial connection    
  Serial.begin(115200);
  //rtc_test.goodwifiap = 0;
  //rtc_test.lastbatt = 0;
  // wait for serial monitor to open
  while(!Serial);
  Serial.println();

  /*
  // pull saved data from RTC memory
  Serial.print("Reading RTC_MEM backup... ");
  if(system_rtc_mem_read(64, &rtc_test, sizeof(rtc_test))){
    Serial.println("Successful!");
    Serial.print("Last Good AP: "); Serial.println(rtc_test.goodwifiap);
    Serial.print("Last Batt: "); Serial.println(rtc_test.lastbatt);
  } else {
    Serial.println("Failed!");
  }
  */
  // wait for led matrix to initialize
  if (! matrix.begin()) {
    Serial.println("IS31 board not found!");
    while (1);
  }

  //get connected to WiFi
  if(!wifi_connect()) {
    Serial.println();
    Serial.println("Failed to connect to WiFi. Shutting down.");
    return; //abort setup() and go to shutdown.
  }
  Serial.println();
  Serial.println("WiFi Connected.");
  Serial.print("IP address: ");Serial.println(WiFi.localIP());Serial.println();
  if(!mqtt.subscribe(&heart)) Serial.println("Failed to subscribe to Ping topic!");
  if (!mqtt.connected()) {
    Serial.print("Connecting to MQTT... ");
    if(mqtt.connect() != 0) { // connect will return 0 for connected, am I sure this comparison will call connect()?
      return; //send to loop() which will kill power
    }
  } 
  Serial.println("success!");
  
  //read Vcc and send to AIO
  uint16_t current = ESP.getVcc(); //read current Vcc on ESP8266
  Serial.println();Serial.println("Reading Vcc on ESP8266.");
  Serial.print("VCC: ");Serial.print(current);Serial.println("mV");
  /*(if(current != rtc_test.lastbatt) { //if it's diff from the last reading, write to RTC memory and AIO
    //Serial.print("sending -> ");
    //Serial.print(current);
    rtc_test.lastbatt = current;
    if(system_rtc_mem_write(64, &rtc_test, sizeof(rtc_test))){
      Serial.println();
      Serial.println("Backup to RTC_MEM successful!");
      //Serial.print("Wrote Last Good AP: "); Serial.println(rtc_test.goodwifiap);
      //Serial.print("Wrote Last Batt: "); Serial.println(rtc_test.lastbatt);
      //Serial.print("Wrote length (bytes): "); Serial.println(sizeof(rtc_test));
    } else {
      Serial.println();
      Serial.println("Backup to RTC_MEM FAILED!");
    }*/      
  batt.publish(current);

  Adafruit_MQTT_Subscribe *subscription;
  while (subscription = mqtt.readSubscription(1000)) {
    if (subscription == &heart) {
      char *value = (char *)heart.lastread;
      Serial.print("Unconverted last heart value: "); Serial.println(value);
      uint16_t lastPing = atoi(value);  // convert to a number
      Serial.print("Last heart value: ");Serial.println(lastPing);
      if(lastPing == 1){
        Serial.println();
        Serial.println("Pinged! Draw Hearts!");
        drawhearts();
        heartClear.publish(0);
      }  
    }
  }
  Serial.println("Going to deep sleep for 5min.");
}

void loop() {
  digitalWrite(DONE_PIN, LOW);
  delay(100);
  digitalWrite(DONE_PIN, HIGH);
  delay(500);
}
