#include <Arduino.h>
#include <WiFi.h>
#include "freertos/event_groups.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>

#define SSID "admin"
#define PASSWORD "domestos1216"
#define WIFI_UPDATE 2000 // 2 секунди

// Біт події "WiFi підключено"
#define WIFI_CONNECTED_BIT BIT0

EventGroupHandle_t wifiEventGroup;
QueueHandle_t dataQueue; // Черга для обміну між задачами

void connectToWiFi(void *pvParameters);
void GreenHouseServer(void *pvParameters);
void webHandle(void *pvParameters);

WiFiServer server1(5000);
WiFiClient client;

WebServer server(80);

float soilLevel = 0;
float hallLevel = 0;
float temperature = 0;
float humidity = 0;
float lightLevel = 0;

void setup()
{
  Serial.begin(115200);
  delay(100);
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  wifiEventGroup = xEventGroupCreate();

  dataQueue = xQueueCreate(10, sizeof(String)); // 10 елементів по типу String
  if (dataQueue == NULL)
  {
    Serial.println("Error creating queue");
  }

  xTaskCreatePinnedToCore(connectToWiFi, "Connect to WiFi", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(GreenHouseServer, "Start wifi server", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(webHandle, "Start wifi server", 4096, NULL, 1, NULL, 0);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}

void connectToWiFi(void *pvParameters)
{
  WiFi.mode(WIFI_STA);

  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi still connected");
      vTaskDelay(pdMS_TO_TICKS(10000));
      continue;
    }

    Serial.println("WiFi Connecting...");
    WiFi.begin(SSID, PASSWORD);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - startTime < WIFI_UPDATE)
    {
      Serial.print(".");
      vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("\nWiFi Failed. Retrying...");
      vTaskDelay(pdMS_TO_TICKS(20000));
      continue;
    }

    Serial.print("\nWiFi Connected. IP: ");
    Serial.println(WiFi.localIP());

    // Дати сигнал серверу, що Wi-Fi готовий
    xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
  }
}

void GreenHouseServer(void *pvParameters)
{
  for (;;)
  {
    // Чекаємо підключення Wi-Fi
    xEventGroupWaitBits(wifiEventGroup, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    // Стартуємо сервер
    server1.begin();
    Serial.println("Server started on port 5000");

    for (;;)
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("WiFi lost. Waiting to reconnect...");
        server1.stop();
        xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT);
        break; // вихід з внутрішнього циклу, щоб знову чекати Wi-Fi
      }

      if (!client || !client.connected())
      {
        client = server1.available();
        if (client)
        {
          Serial.println("Client connected");
        }
      }

      if (client && client.connected() && client.available())
      {
        String msg = client.readStringUntil('\n');
        msg.trim();
        Serial.print("Client says: ");
        Serial.println(msg);

        if (xQueueSend(dataQueue, &msg, 0) != pdPASS)
        {
          Serial.println("Queue full, message dropped");
        }
      }

      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
}

void handleRoot()
{
  if (LittleFS.exists("/index.html"))
  {
    File file = LittleFS.open("/index.html", "r");
    if (file)
    {
      server.streamFile(file, "text/html");
      file.close();
      return;
    }
  }
  server.send(404, "text/plain", "File not found");
}

void handleData()
{
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["soilLevel"] = soilLevel;
  jsonDoc["hallLevel"] = hallLevel;
  jsonDoc["temp_c"] = temperature;
  jsonDoc["humidity"] = humidity;
  jsonDoc["light"] = lightLevel;

  String response;
  serializeJson(jsonDoc, response);
  server.send(200, "application/json", response);
}

void webHandle(void *pvParameters)
{
  // Реєструємо хендлери
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);

  server.begin();
  Serial.println("HTTP server started");

  String receivedData;

  for (;;)
  {
    server.handleClient(); // обробка HTTP запитів

    // Чекаємо нові дані з черги
    if (xQueueReceive(dataQueue, &receivedData, 0) == pdTRUE)
    {
      Serial.print("Processing data: ");
      Serial.println(receivedData);

      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, receivedData);
      if (!error)
      {
        soilLevel = doc["soilLevel"] | soilLevel;
        hallLevel = doc["hallLevel"] | hallLevel;
        temperature = doc["temp_c"] | temperature;
        humidity = doc["humidity"] | humidity;
        lightLevel = doc["light"] | lightLevel;
      }
      else
      {
        Serial.println("JSON parse error");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // маленька пауза для FreeRTOS
  }
}
