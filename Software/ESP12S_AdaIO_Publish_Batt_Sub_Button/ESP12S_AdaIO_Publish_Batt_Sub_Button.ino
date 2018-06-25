// Adafruit IO Publish Example
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

/************************ Example Starts Here *******************************/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>

#define ADC_PIN A0
#define LED_PIN 2

float current = 0;
float last = -1;

AdafruitIO_Feed *batt = io.feed("batt");
AdafruitIO_Feed *light = io.feed("light");
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


void setup() {

  // start the serial connection
  pinMode(LED_PIN, OUTPUT);
  // onboard LED is wired backwards? high is off, low is on?
  digitalWrite(LED_PIN, HIGH);
    
  Serial.begin(115200);

  // wait for serial monitor to open
  //while(! Serial);
  // wait for led matrix to initialize
  if (! matrix.begin()) {
    while (1);
  }

  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();

  light->onMessage(handleLight);

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}

void loop() {

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
  current = analogRead(ADC_PIN);
  current = current/1023.00*6.00;
  //current = map(current, 0, 1023, 0, 6);
  if(current == last)
    return;
  
  // save current to the 'batt' feed on Adafruit IO
  Serial.print("sending -> ");
  Serial.println(current);
  batt->save(current);
  last = current;

  matrix.setRotation(0);
  matrix.clear();
  for (int8_t x=36; x>=-36; x--) {
    matrix.clear();
    matrix.drawBitmap(x,0, hearts, 36, 8, 255/4);
    delay(25);
  }
  // wait one second (1000 milliseconds == 1 second)
  delay(5000);
  
}

//onboard LED is backwards.  High is off, low is on.
void handleLight(AdafruitIO_Data *data) {
  Serial.print("received <- light ");
  if(data->isTrue()) {
    Serial.println("is on.");
    digitalWrite(LED_PIN, LOW);
  }
  else {
    Serial.println("is off.");
    digitalWrite(LED_PIN, HIGH);
  }
}

