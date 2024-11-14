#include "FileConfig.h"

// AT24Cxx eep(0x57, 32);

//////////////////////////////////////////////
// Loading settings from config.json file   //
//////////////////////////////////////////////
void LoadConfig()
{
  // Serial.println("Loading Configuration from config.json");
  String jsonConfig = "";

  File configFile = SPIFFS.open("/config.json", "r"); // Открываем файл для чтения
  jsonConfig = configFile.readString();               // загружаем файл конфигурации из EEPROM в глобальную переменную JsonObject
  configFile.close();

  StaticJsonDocument<1024> doc; //  Резервируем памяь для json обекта
  // Дессиарилизация - преобразование строки в родной объкт JavaScript (или преобразование последовательной формы в параллельную)
  deserializeJson(doc, jsonConfig); //  вызовите парсер JSON через экземпляр jsonBuffer

  char TempBuf[15];

  struct _sys
  {
    uint8_t H = 0;
    uint8_t M = 0;
  } SMS1, SMS2;

  sensors.averange = doc["avr"];
  sensors.calib = doc["cal"];
  CFG.fw = doc["firmware"].as<String>();
  CFG.IP1 = doc["ip1"];
  CFG.IP2 = doc["ip2"];
  CFG.IP3 = doc["ip3"];
  CFG.IP4 = doc["ip4"];
  CFG.APPAS = doc["pass"].as<String>();
  CFG.phone = doc["phone"].as<String>();

  doc["sms1"].as<String>().toCharArray(TempBuf, 10);
  SMS1.H = atoi(strtok(TempBuf, ":"));
  SMS1.M = atoi(strtok(NULL, ":"));
  CFG.UserSendTime1 = SMS1.H;

  doc["sms2"].as<String>().toCharArray(TempBuf, 10);
  SMS2.H = atoi(strtok(TempBuf, ":"));
  SMS2.M = atoi(strtok(NULL, ":"));
  CFG.UserSendTime2 = SMS2.H;

  CFG.sn = doc["sn"];

  CFG.APSSID = doc["ssid"].as<String>();

  sensors.T1_offset = doc["t1_offset"];
  sensors.T2_offset = doc["t2_offset"];

  ST.WiFiEnable = doc["wifi_st"];
}

void ShowLoadJSONConfig()
{
  char msg[60] = {0}; // buff for send message

  Serial.println(F("##############  System Configuration  ###############"));
  Serial.println("-------------------- USER DATA -----------------------");
  Serial.printf("####  Phone: %s \r\n", CFG.phone);
  Serial.printf("####  SMS1: %d \r\n", CFG.UserSendTime1);
  Serial.printf("####  SMS2: %d \r\n", CFG.UserSendTime2);
  Serial.printf("####  T1_OFFSET: %d \r\n", sensors.T1_offset);
  Serial.printf("####  T2_OFFSET: %d \r\n", sensors.T2_offset);
  sprintf(msg, "####  CAL: %0.5f  | W_AVR: %0d \r\n", sensors.calib, sensors.averange);
  Serial.println(F(msg));
  Serial.println("------------------ USER DATA END----------------------");
  Serial.println();
  Serial.println("---------------------- SYSTEM ------------------------");
  sprintf(msg, "####  DATA: %0002d-%02d-%02d", Clock.year, Clock.month, Clock.date);
  Serial.println(F(msg));
  sprintf(msg, "####  TIME: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
  Serial.println(F(msg));
  Serial.printf("####  WiFI_PWR: %d \r\n", ST.WiFiEnable);
  Serial.printf("####  WiFI NAME: %s \r\n", CFG.APSSID);
  Serial.printf("####  WiFI PASS: %s \r\n", CFG.APPAS);
  sprintf(msg, "####  IP: %00d.%00d.%00d.%00d", CFG.IP1, CFG.IP2, CFG.IP3, CFG.IP4);
  Serial.println(F(msg));
  Serial.printf("####  SN: %d", CFG.sn);
  Serial.printf(" FW:");
  Serial.print(CFG.fw);
  Serial.println();
  Serial.println("-------------------- SYSTEM END-----------------------");
  Serial.println(F("######################################################"));
}

//////////////////////////////////////////////
// Save configuration in config.json file   //
//////////////////////////////////////////////
void SaveConfig()
{
  String jsonConfig = "";
  StaticJsonDocument<1024> doc;

  // doc["carname"] = String(UserText.carname);
  doc["avr"] = sensors.averange;
  doc["cal"] = sensors.calib;
  doc["firmware"] = CFG.fw;
  doc["ip1"] = CFG.IP1;
  doc["ip2"] = CFG.IP2;
  doc["ip3"] = CFG.IP3;
  doc["ip4"] = CFG.IP4;
  doc["pass"] = CFG.APPAS;
  doc["phone"] = CFG.phone;

  doc["sn"] = CFG.sn;
  doc["ssid"] = CFG.APSSID;
  doc["t1_offset"] = sensors.T1_offset;
  doc["t2_offset"] = sensors.T2_offset;
  doc["wifi_st"] = ST.WiFiEnable;

  File configFile = SPIFFS.open("/config.json", "w");
  serializeJson(doc, configFile); // Writing json string to file
  configFile.close();

  Serial.println("Configuration SAVE");
}

void TestDeserializJSON()
{
  String jsonConfig = "";

  File configFile = SPIFFS.open("/config.json", "r"); // Открываем файл для чтения
  jsonConfig = configFile.readString();               // загружаем файл конфигурации из EEPROM в глобальную переменную JsonObject
  configFile.close();

  StaticJsonDocument<2048> doc;
  deserializeJson(doc, jsonConfig); //  вызовите парсер JSON через экземпляр jsonBuffer

  serializeJsonPretty(doc, Serial);
  Serial.println();

  Serial.println("JSON testing comleted");
}
