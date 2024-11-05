#include <main.h>

AsyncWebServer server(80);

Adafruit_BME280 bme;

float temp;
float pres;
float humi;

unsigned long previousMillis, startCountTime, ledMillis;

unsigned long counts = 0;
float cpm = 0;
int status;

Servo sg90;
int speed0 = 92, pos;


void setup() {
  Serial.begin(115200);
  pinMode(GEIGER_PIN, INPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);


  Serial.println("Creating access point!!");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASS);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  Serial.println(WiFi.localIP());
  delay(100);

  config_api();

  Wire.setPins(SDA_PIN, SCL_PIN);
  bool status = bme.begin(0x77, &Wire);
  if (status)
  {
    Serial.println("BME280 sensor found!");
  }
  else
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring! Resetting in 5 s...");
    unsigned long time_now = millis();
    while (millis() - time_now < 5000)
    {
    }
    ESP.restart();
  }

  temp = bme.readTemperature();
  pres = bme.readPressure() / 100.0F;
  humi = bme.readHumidity();

  sg90.setPeriodHertz(50); // PWM frequency for SG90
  sg90.attach(SG90_PIN, 500, 2400); // Minimum and maximum pulse width (in µs) to go from 0° to 180
  pos = speed0;
  sg90.write(pos);

  startCountTime = millis();
  Serial.println("Starting the counting...");
  delay(10);
}

void loop() {
  if ((millis() - previousMillis) > 10) {
    if (status == 1) {
      cpm = (float) (60000 * counts) / (millis() - startCountTime);
    }
  }
  if ((millis() - previousMillis) > 1000)
  {
    previousMillis = millis();
    read_env();
    Serial.printf("Counts: %d, CPM: %.2f\n", counts, cpm);
    Serial.printf("Temp: %.2f, Pres: %.2f, Humi: %.2f\n", temp, pres, humi);
    Serial.printf("Servo Speed: %d\n", pos-speed0);
    Serial.printf("LED1: %d, LED2: %d\n", digitalRead(LED1_PIN), digitalRead(LED2_PIN));
    Serial.println("****************************************************");
  }

}

void config_api() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
        { request->send(200, "application/json", "{\"Hello\": \"World\"}"); });

  server.on("/geiger", HTTP_GET, [](AsyncWebServerRequest *request)
        { 
          char answer[70];
          snprintf(answer, sizeof(answer), "{\"counts\": %d, \"cpm\": %.2f, \"status\": %d}", counts, cpm, status);
          request->send(200, "application/json", answer); 
          });

  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request)
        { 
          if (status == 0) {
            status = 1;
            startCountTime = millis();
            counts = 0;
            cpm = 0;
            attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), ISR_impulse, RISING);
          }
          
          char answer[70];
          snprintf(answer, sizeof(answer), "{\"counts\": %d, \"cpm\": %.2f, \"status\": %d}", counts, cpm, status);
          request->send(200, "application/json", answer); 
          });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request)
        { 
          if (status == 1) {
            status = 0;
            startCountTime = millis();

            detachInterrupt(digitalPinToInterrupt(GEIGER_PIN));
          }
          
          char answer[70];
          snprintf(answer, sizeof(answer), "{\"counts\": %d, \"cpm\": %.2f, \"status\": %d}", counts, cpm, status);
          request->send(200, "application/json", answer); 
          });
          
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
        { 
          detachInterrupt(digitalPinToInterrupt(GEIGER_PIN));
          counts = 0;
          cpm = 0;
          status = 1;
          attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), ISR_impulse, RISING);
          startCountTime = millis();

          char answer[70];
          snprintf(answer, sizeof(answer), "{\"counts\": %d, \"cpm\": %.2f, \"status\": %d}", counts, cpm, status);
          request->send(200, "application/json", answer); 
          });

  server.on("/env", HTTP_GET, [](AsyncWebServerRequest *request)
        { 
          char answer[60];
          snprintf(answer, sizeof(answer), "{\"temp\": %.2f, \"pres\": %.2f, \"humi\": %.2f}", temp, pres, humi);
          request->send(200, "application/json", answer); 
          });

  server.on("/motor", HTTP_GET, [](AsyncWebServerRequest *request)
        { 
          char answer[30];
          snprintf(answer, sizeof(answer), "{\"speed\": %d}", pos - speed0);
          request->send(200, "application/json", answer); 
          });

  server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request)
        { 
          char answer[50];
          snprintf(answer, sizeof(answer), "{\"led1\": %d, \"led2\": %d}", digitalRead(LED1_PIN), digitalRead(LED2_PIN));
          request->send(200, "application/json", answer); 
          });
          
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  if (request->url() == "/motor") {
    JsonDocument doc;
    deserializeJson(doc, data);
    if (doc["speed"].is<int>()) {
      int speed = doc["speed"];
      pos = speed + speed0;
      if (pos > 180) {
        pos = 180;
      }
      if (pos < 0) {
        pos = 0;
      }
      sg90.write(pos);
    }
    
    char answer[30];
    snprintf(answer, sizeof(answer), "{\"speed\": %d}", pos-speed0);
    request->send(200, "application/json", answer); 
    }
  else if (request->url() == "/led") {
    JsonDocument doc;
    deserializeJson(doc, data);
    if (doc["led1"].is<int>()) {
      int state = doc["led1"];
      digitalWrite(LED1_PIN, state);
    }
    if (doc["led2"].is<int>()) {
      int state = doc["led2"];
      digitalWrite(LED2_PIN, state);
    }

    char answer[50];
    snprintf(answer, sizeof(answer), "{\"led1\": %d, \"led2\": %d}", digitalRead(LED1_PIN), digitalRead(LED2_PIN));
    request->send(200, "application/json", answer); 
  }
  });

  server.onNotFound([](AsyncWebServerRequest *request)
        { request->send(404, "application/json", "{\"Not\": \"Found\"}"); });
  server.begin();
}

void read_env() {
  temp = bme.readTemperature();
  pres = bme.readPressure() / 100.0F;
  humi = bme.readHumidity();
}

void IRAM_ATTR ISR_impulse() { // Captures count of events from Geiger counter board
  counts++;
}
