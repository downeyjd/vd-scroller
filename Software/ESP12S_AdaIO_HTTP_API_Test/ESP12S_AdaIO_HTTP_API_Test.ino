#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

//adafruit.io credentials
#define AIO_KEY          "5eb2369140e04b4abaa5bfd270718841"
#define HEART_LAST      "https://io.adafruit.com/api/v2/downeyjd/feeds/heart/data/last"
#define HEART_DATA      "https://io.adafruit.com/api/v2/downeyjd/feeds/heart/data"
#define BATT_DATA       "https://io.adafruit.com/api/v2/downeyjd/feeds/batt/data"
#define DONE_PIN        12
const char* fingerprint = "77 00 54 2d da e7 d8 03 27 31 23 99 eb 27 db cb a5 4c 57 18";

ADC_MODE(ADC_VCC); //setup ESP8266 ADC to measure VCC and NOT use A0 pin
char* wifiaps[2][2] = {{"bleepblorp", "1234"}, {"blerg", "VUrocks00!!"}};
WiFiClient client;
HTTPClient http;

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
  
  //read Vcc and send to AIO
  uint16_t current = ESP.getVcc(); //read current Vcc on ESP8266
  Serial.println();Serial.println("Reading Vcc on ESP8266.");
  Serial.print("VCC: ");Serial.print(current);Serial.println("mV");
  //batt.publish(current);
  String sendBatt = "{\"value\": \"";
  sendBatt = sendBatt + current;
  sendBatt = sendBatt + "\"}";
  http.begin(BATT_DATA, fingerprint);
  http.addHeader("X-AIO-Key", AIO_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("NULL", "NULL");
  http.POST(sendBatt);
  http.writeToStream(&Serial);
  http.end();  
  http.begin(HEART_LAST, fingerprint);
  http.addHeader("X-AIO-Key", AIO_KEY);
  int httpCode = http.GET();
  if(httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    http.end();
    Serial.println(payload);
    const size_t bufferSize = JSON_OBJECT_SIZE(10) + 210;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.parseObject(payload);
    uint8_t heartflag = atoi(root["value"]);
    Serial.print("Last Heart value: ");Serial.println(heartflag);
    if(heartflag == 1) {
      Serial.println();
      Serial.println("Pinged! Draw Hearts!");
      drawhearts();
      //heartClear.publish(0);
      String clearHeart = "{\"value\": \"0\"}";
      http.begin(HEART_DATA, fingerprint);
      http.addHeader("X-AIO-Key", AIO_KEY);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("NULL", "NULL");
      http.POST(clearHeart);
      http.writeToStream(&Serial);
      http.end(); 
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
