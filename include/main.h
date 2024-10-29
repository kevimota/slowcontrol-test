#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "ArduinoJson.h"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <ESP32Servo.h>

#include <esp_task_wdt.h>

#define WDT_TIMEOUT 120

#define LED1_PIN 13
#define LED2_PIN 14

#define SDA_PIN 40
#define SCL_PIN 41

#define GEIGER_PIN 46

#define SG90_PIN 45

#define SSID "Dirijo"
#define PASS "qkay6139"

void IRAM_ATTR Timer0_ISR();
void read_env();
void IRAM_ATTR ISR_impulse();
void config_api();