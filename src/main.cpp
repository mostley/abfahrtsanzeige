#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

const char* WIFI_SSID = "";
const char* WIFI_PSK = "";

const char* API_KEY = "377d840e54b59adbe53608ba1aad70e8";
const String API_SSL_FINGERPRINT = "4B:57:8A:E0:CA:B6:54:14:EE:49:71:12:4C:FC:64:01:AF:15:15:CD";
const char* haltestelle = "de:8212:1207";
//const char* haltestelle = "de:8212:89"; // HBF
const String API_BASE = "live.kvv.de";
const String API_PATH = "/webapp/departures/bystop/" + String(haltestelle) + "?key=" + String(API_KEY) + "&maxInfos=10";

ESP8266WiFiMulti WiFiMulti;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);


void pre(void)
{
  u8g2.setFont(u8g2_font_amstrad_cpc_extended_8f);
  u8g2.clear();

  u8g2.drawBox(0, 0, 128, 8);
  u8g2.setDrawColor(0);
  u8g2.setCursor(3, 8);
  u8g2.print("  Haltestelle  ");
  u8g2.setFont(u8g2_font_chroma48medium8_8r);
  u8g2.setDrawColor(1);
}

void draw_bar(uint8_t c, uint8_t is_inverse)
{
  u8g2.setDrawColor(is_inverse);
  for(uint8_t r = 0; r < 64; r++ )
  {
    u8g2.setCursor(c, r*8);
    u8g2.print(" ");
  }
}

String getData() {
  String url = "https://" + API_BASE + API_PATH;

  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(API_BASE);
  if (!client.connect(API_BASE.c_str(), 443)) {
    Serial.println("connection failed");
    return "";
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
           "Host: " + API_BASE + "\r\n" +
           "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36 x-chrome-uma-enabled:1\r\n" +
           "Connection: close\r\n\r\n");

  Serial.println("request sent");

  String line;
  while (client.connected()) {
    line = client.readStringUntil('\n');
    if (line.startsWith("{")) {
      Serial.println("data received");
      break;
    }
  }
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");

  return line;
}

void setup(void)
{
  Serial.begin(115200);

  u8g2.begin();

  Serial.println("connect to WiFi network");
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PSK);
  Serial.println("[HTTP] waiting for connection");

  u8g2.firstPage();

  pre();
  u8g2.setFont(u8g2_font_crox5tb_tf);
  u8g2.setCursor(33, 35);
  u8g2.print("KVV");
  u8g2.setCursor(18, 55);
  u8g2.print("anzapfen");
  u8g2.nextPage();
}

void printDepartures(JsonArray& departures) {
    u8g2.setFont(u8g2_font_inb63_mn);
    u8g2.clear();

    if (departures.size() > 0) {
      int departureIndex = 0;
      String checkTime = departures[0]["time"];
      if (departures.size() > 1 && (checkTime.endsWith("0") || checkTime.endsWith("1 min") || checkTime.endsWith("2 min") || checkTime.endsWith("3 min"))) {
        departureIndex = 1;
      }
      JsonObject& departure = departures[departureIndex];

      String dir = departure["direction"];
      String firstTime = departure["time"];
      bool goingToKarlsruhe = dir.startsWith("1");
      bool isTime = !firstTime.endsWith("min") && !firstTime.endsWith("0");

      if (goingToKarlsruhe) {
        u8g2.setDrawColor(1);
      } else {
        u8g2.setDrawColor(0);
      }

      if (isTime) {
        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_logisoso30_tf);
        u8g2.setCursor(0, 50);
        String direction = goingToKarlsruhe ? " >" : " <";
        u8g2.print(firstTime + direction);
        u8g2.nextPage();

        delay(15000);
      } else {
        bool isCritical = firstTime.endsWith("4 min") || firstTime.endsWith("3 min") || firstTime.endsWith("2 min") || firstTime.endsWith("1 min") || firstTime.endsWith("0");

        if (isCritical) {
          bool toggle = false;
          for (int j=0; j<15; j++) {
            if (goingToKarlsruhe) {
              u8g2.setDrawColor(1);
            } else {
              u8g2.setDrawColor(0);
            }

            u8g2.clear();
            u8g2.setCursor(40, 63);
            u8g2.print(firstTime.substring(0, 1));
            if (toggle) {
              u8g2.setDrawColor(1);
              u8g2.drawFrame(0,0,127,64);
              u8g2.drawFrame(1,1,126,63);
            }
            toggle = !toggle;
            u8g2.nextPage();
            delay(1000);
          }
        } else {
          u8g2.setCursor(40, 63);
          u8g2.print(firstTime.substring(0, 1));
          u8g2.nextPage();

          delay(15000);
        }
      }
    } else {
      u8g2.setFont(u8g2_font_amstrad_cpc_extended_8f);
      u8g2.setCursor(0, 20);
      u8g2.print("- keine Z\xfcge -"); // todo font size
      u8g2.nextPage();

      delay(15000);
    }
}

void loop(void)
{
  if ( WiFiMulti.run() == WL_CONNECTED ) {
    pre();
    u8g2.setFont(u8g2_font_fub20_tf);
    u8g2.setCursor(20, 50);
    u8g2.print("Update");
    u8g2.nextPage();

    String data = getData();

    DynamicJsonBuffer jsonBuffer(JSON_ARRAY_SIZE(10) + JSON_OBJECT_SIZE(3) + 10*JSON_OBJECT_SIZE(9));

    JsonObject& root = jsonBuffer.parseObject(data);

    if (root.success()) {
      JsonArray& departures = root["departures"];
      printDepartures(departures);
    } else {
      Serial.println("JSON parsing failed!");

      pre();
      u8g2.setCursor(0, 20);
      u8g2.print("Error! :-(");
      u8g2.nextPage();
    }

  } else {
    Serial.println("[HTTP] not yet connected");
  }
}
