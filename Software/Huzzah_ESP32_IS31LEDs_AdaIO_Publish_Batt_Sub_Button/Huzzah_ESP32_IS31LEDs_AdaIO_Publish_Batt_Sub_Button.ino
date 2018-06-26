#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>
#include "AdafruitIO_WiFi.h"

#define IO_USERNAME     "downeyjd"
#define IO_KEY          "5eb2369140e04b4abaa5bfd270718841"
#define WIFI_SSID       "blerg"
#define WIFI_PASS       "VUrocks00!!"
#define VOLT_PIN        A13

uint16_t current = 0;
uint16_t last = 0;

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *heart = io.feed("heart");
AdafruitIO_Feed *battery = io.feed("batt");

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

void setup() {
  // start the serial connection    
  Serial.begin(115200);
  while(!Serial);
  Serial.println();

  // wait for led matrix to initialize
  if (! matrix.begin()) {
    Serial.println("IS31 board not found!");
    while (1);
  }
  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();
  heart->onMessage(handleMessage);
}

void loop() {
  io.run();
  
  //read Vcc and send to AIO if new
  uint16_t current = analogRead(VOLT_PIN); //read current Vcc on ESP8266
  current *= 2; //voltage divider setup to half voltage going to pin, so double it.
  Serial.println();Serial.println("Reading Vcc on ESP32.");
  Serial.print("VCC: ");Serial.print(current);Serial.println("mV");
  if(current != last) { //if it's diff from the last reading, write to AIO
    Serial.print("sending -> ");
    Serial.print(current);
    last = current;
    battery->save(current);
  }
  delay(5000);
}

void handleMessage(AdafruitIO_Data *data) {
  Serial.print("received <- ");
  Serial.println(data->value());
  drawhearts();
}
