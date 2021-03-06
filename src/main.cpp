/*
  ESP32 Color music
  Copyright (c) 2021 Ignatiy Vakorin. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include<MegunoLink.h>
#include<Filter.h>
#include <GyverButton.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>



/* Initial setup begin */
#define WIFI_LED_PIN  2
// MIC
#define MIC_PIN 36 // Укажите пин подключения микрофона
// LED
#define LED_PIN 23 // Укажите пин подключения светодиодной ленты
#define NUM_LEDS 46 // Укажите кол-во светодиодов
#define LED_TYPE WS2812B // Укажите тип ленты
#define BRIGHTNESS  100 // Укажите максимальную яркость
#define COLOR_ORDER GRB

#define NOISE 500 // Уровень чувствительности микрофона от 0 до 1000, больше значение меньше чувствительность
#define TOP (NUM_LEDS+2)
CRGB leds[NUM_LEDS];
// BUTTON 
#define BUTTON 22
GButton button(BUTTON, NORM_CLOSE);

// Effects 
#define MODE 0 
byte this_mode = MODE; 


byte MAX_CH = NUM_LEDS / 2;
byte count;
int hue;
unsigned long main_timer, hue_timer;
int n, height;
#define RAINBOW_SPEED 6

int lvl = 0, minLvl = 0, maxLvl = 300;
ExponentialFilter<long> ADCFilter(5,0);
/* Initial setup end */



/* Wi-Fi setup begin */
const char* pass = "password"; //WiFi pass
const char* ssid = "ssid"; 
/* Wi-Fi setup end */

// WEB SERVER
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setup() {
    Serial.begin(9600);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();

    pinMode(WIFI_LED_PIN, OUTPUT);
    button.setTimeout(900);
    digitalWrite(WIFI_LED_PIN, LOW);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, pass);
    while(WiFi.status() !=WL_CONNECTED )
    {
      digitalWrite(WIFI_LED_PIN, HIGH);
      delay(250);
      digitalWrite(WIFI_LED_PIN, LOW);
      delay(250);
      Serial.println("Waiting for connect");
    }
    if (WiFi.isConnected()) digitalWrite(WIFI_LED_PIN, HIGH);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      request->send(200, "text/plain", "Hi! This is test for OTA");
    });
    AsyncElegantOTA.begin(&server);
    server.begin();
    Serial.println("HTTP server started");
    Serial.println(WiFi.localIP()); 
}


CRGB Wheel(byte WheelPos) {
  // return a color value based on an input value between 0 and 255
  if(WheelPos < 85)
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void effects()
{
  FastLED.setBrightness(BRIGHTNESS); 
  switch(this_mode)
  {
    case 2:
      count = 0;
      for(uint8_t i = 0; i < NUM_LEDS; i++) {
        if(i >= height) leds[i] = CRGB(0,0,0);
        else leds[i] =  ColorFromPalette(RainbowColors_p, (count * ((float)255 / MAX_CH)) / 2 - hue);
      count ++;
      }
      break;
    case 3:
      count = 0;
      for(uint8_t i = 0; i < NUM_LEDS; i++) {
        if(i >= height) leds[i] = CRGB(0,0,0);
        else leds[i] = Wheel( map( i, 0, NUM_LEDS-1, 30, 150 ) );
      count ++;
      }
      break;
  }
}


void loop() {
  AsyncElegantOTA.loop();
  button.tick();
  if (button.isSingle())
  {
    if (++this_mode > 3) this_mode = 0;
    switch(this_mode)
    {
      case 0:
        Serial.println("Led switched Off");
        break;
      case 1:
        Serial.println("Led in mirror light mode");
        break;
      case 2:
        Serial.println("Led in rainbow mode");
        break;
      case 3:
        Serial.println("Led in RGB strobe mode");
        break;
    }
  }
  if (button.isDouble())
  {
    if (this_mode == 2) this_mode = 3;
    else if (this_mode == 3) this_mode = 2;
  }
  if (button.isHolded())
  {
    if (++this_mode > 3) this_mode = 0;
    switch(this_mode)
    {
      case 0:
        Serial.println("Led switched Off");
        break;
      case 1:
        Serial.println("Led in mirror light mode");
        break;
      case 2:
        Serial.println("Led in rainbow mode");
        break;
      case 3:
        Serial.println("Led in RGB strobe mode");
        break;
    }
  }
  // {
  //   if (++this_mode > 3) this_mode = 0;
  //   switch (this_mode)
  //   {
  //     case 0:
  //       this_mode = 1;
  //       break;
  //     case 1:
  //       this_mode = 0;
  //       break;
  //     case 2:
  //       this_mode = 1;
  //       break;
  //     case 3:
  //       this_mode = 1; 
  //       break; 
  //   }
  // }
  if (this_mode == 0){
    FastLED.setBrightness(0);
  }
  else if (this_mode == 1)
  {
    FastLED.setBrightness(BRIGHTNESS);
    for(int i = 0; i<=NUM_LEDS; i++)
    {
      leds[i] = CRGB(255,255,255); 
    }
  }
  else 
  {
    if (millis() - hue_timer > RAINBOW_SPEED) 
    {
      if (++hue >= 255) hue = 0;
      hue_timer = millis();
    }
    n = analogRead(MIC_PIN);
    n = abs(1023 - n);
    n = (n <= NOISE) ? 0 : abs(n - NOISE);
    ADCFilter.Filter(n);
    lvl = ADCFilter.Current();
    height = TOP * (lvl - minLvl) / (long)(maxLvl - minLvl);
    if(height < 0L) height = 0;
    else if(height > TOP) height = TOP;
    effects();
  }
  FastLED.show();           // отправить значения на ленту
  FastLED.clear();          // очистить массив пикселей
  main_timer = millis();    // сбросить таймер
}