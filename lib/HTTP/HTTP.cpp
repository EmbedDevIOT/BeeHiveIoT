#include "HTTP.h"

WebServer HTTP(80);

///////////////////////////////
// Initialisation WebServer  //
///////////////////////////////
void HTTPinit()
{
  HTTP.begin();
  // ElegantOTA.begin(&HTTP); // Start ElegantOTA
  HTTP.on("/update.json", UpdateData); // Update Time and Data
  HTTP.on("/state.json", UpdateState); // Update BeeHive state (request every 1s)
  HTTP.on("/NotUPD", NotificationUPD); // Save and Update notification users data (time_sms1, time_sms2, phone_number)
  HTTP.on("/SysUPD", SystemUpdate);    // Save System settings (temperature T1,T2 offset)
  HTTP.on("/TimeUPD", TimeUpdate);     // RTC Data update
  HTTP.on("/WSetZero", ScaleSetZero);  // Scale set zero.
  HTTP.on("/WCalUPD", ScaleCalSave);   // Scale save cafibration factor.
  HTTP.on("/WiFiUPD", SaveSecurity);   // Save WiFi data (SSID and PASS)
  HTTP.on("/BRBT", Restart);           // Restart MCU
  HTTP.on("/SNUPD", SerialNumberUPD);  // Service. Get Request Serial Number Update
  HTTP.on("/FW", ShowSystemInfo);      // Service. Show Serial number and Firmware version
  HTTP.on("/BFRST", FactoryReset);     // Set default parametrs.

  HTTP.onNotFound([]() {                         // Event "Not Found"
    if (!handleFileRead(HTTP.uri()))             // If function  handleFileRead (discription bellow) returned false in request for file searching in file syste
      HTTP.send(404, "text/plain", "Not Found"); // return message "File isn't found" error state 404 (not found )
  });
}

////////////////////////////////////////
// File System work handler           //
////////////////////////////////////////
bool handleFileRead(String path)
{
  if (path.endsWith("/"))
    path += "index.html";                    // Если устройство вызывается по корневому адресу, то должен вызываться файл index.html (добавляем его в конец адреса)
  String contentType = getContentType(path); // С помощью функции getContentType (описана ниже) определяем по типу файла (в адресе обращения) какой заголовок необходимо возвращать по его вызову
  if (SPIFFS.exists(path))
  {                                                   // Если в файловой системе существует файл по адресу обращения
    File file = SPIFFS.open(path, "r");               //  Открываем файл для чтения
    size_t sent = HTTP.streamFile(file, contentType); //  Выводим содержимое файла по HTTP, указывая заголовок типа содержимого contentType
    file.close();                                     //  Закрываем файл
    return true;                                      //  Завершаем выполнение функции, возвращая результатом ее исполнения true (истина)
  }
  return false; // Завершаем выполнение функции, возвращая результатом ее исполнения false (если не обработалось предыдущее условие)
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Функция, возвращающая необходимый заголовок типа содержимого в зависимости от расширения файла //
////////////////////////////////////////////////////////////////////////////////////////////////////
String getContentType(String filename)
{
  if (filename.endsWith(".html"))
    return "text/html"; // Если файл заканчивается на ".html", то возвращаем заголовок "text/html" и завершаем выполнение функции
  else if (filename.endsWith(".css"))
    return "text/css"; // Если файл заканчивается на ".css", то возвращаем заголовок "text/css" и завершаем выполнение функции
  else if (filename.endsWith(".js"))
    return "application/javascript"; // Если файл заканчивается на ".js", то возвращаем заголовок "application/javascript" и завершаем выполнение функции
  else if (filename.endsWith(".png"))
    return "image/png"; // Если файл заканчивается на ".png", то возвращаем заголовок "image/png" и завершаем выполнение функции
  else if (filename.endsWith(".ttf"))
    return "font/ttf"; // Если файл заканчивается на ".png", то возвращаем заголовок "image/png" и завершаем выполнение функции
  else if (filename.endsWith(".bmp"))
    return "image/bmp";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg"; // Если файл заканчивается на ".jpg", то возвращаем заголовок "image/jpg" и завершаем выполнение функции
  else if (filename.endsWith(".gif"))
    return "image/gif"; // Если файл заканчивается на ".gif", то возвращаем заголовок "image/gif" и завершаем выполнение функции
  else if (filename.endsWith(".svg"))
    return "image/svg+xml";
  else if (filename.endsWith(".ico"))
    return "image/x-icon"; // Если файл заканчивается на ".ico", то возвращаем заголовок "image/x-icon" и завершаем выполнение функции
  return "text/plain";     // Если ни один из типов файла не совпал, то считаем что содержимое файла текстовое, отдаем соответствующий заголовок и завершаем выполнение функции
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// Time data dynamic update
void UpdateData()
{
  String buf = "{";

  buf += "\"t\":\"";
  buf += ((Clock.hour < 10) ? "0" : "") + String(Clock.hour) + ":" + ((Clock.minute < 10) ? "0" : "") + String(Clock.minute) + "\",";
  buf += "\"d\":\"";
  buf += String(Clock.year) + "-" + ((Clock.month < 10) ? "0" : "") + String(Clock.month) + "-" + ((Clock.date < 10) ? "0" : "") + String(Clock.date) + "\"";
  buf += "}";

  HTTP.send(200, "text/plain", buf);
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// BeeHive state dynamic update
void UpdateState()
{
  String buf = "{";

  buf += "\"w\":" + String(sensors.kg) + ",";
  buf += "\"t1\":" + String(sensors.dsT) + ",";
  buf += "\"t2\":" + String(sensors.bmeT) + ",";
  buf += "\"h\":" + String(sensors.bmeH) + ",";
  buf += "\"p\":" + String(sensors.bmeP_mmHg) + ",";
  buf += "\"b\":" + String(sensors.voltage) + ",";
  buf += "\"s\":" + String(sensors.signal);
  buf += "}";

  HTTP.send(200, "text/plain", buf);
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void ScaleSetZero()
{
  // ST.SetZero = true;
  System.DispMenu = ZeroSet;
  Serial.printf("Set Zero \r\n");
  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void ScaleCalSave()
{
  sensors.calib = HTTP.arg("WC").toFloat(); // weight calibration factor

  SaveConfig();

  Serial.printf("Calibration: %f \r\n", sensors.calib);
  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void NotificationUPD()
{
  char TempBuf[15];

  struct _buf
  {
    uint8_t H = 0;
    uint8_t M = 0;
  } SMS1, SMS2;

  HTTP.arg("SMS1").toCharArray(TempBuf, 10);
  SMS1.H = atoi(strtok(TempBuf, ":"));
  SMS1.M = atoi(strtok(NULL, ":"));

  CFG.UserSendTime1 = SMS1.H;
  Serial.printf("EEPROM: SMS_1: %02d \r\n", CFG.UserSendTime1);

  HTTP.arg("SMS2").toCharArray(TempBuf, 10);
  SMS2.H = atoi(strtok(TempBuf, ":"));
  SMS2.M = atoi(strtok(NULL, ":"));

  CFG.UserSendTime2 = SMS2.H;
  Serial.printf("EEPROM: SMS_1: %02d \r\n", CFG.UserSendTime2);

  CFG.phone = HTTP.arg("P");
  Serial.print("Phone Number: ");
  Serial.println(CFG.phone);

  SaveConfig();

  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/
// Time and Date update
void TimeUpdate()
{
  char TempBuf[15];
  char msg[32] = {0};

  struct _sys
  {
    uint8_t H = 0;
    uint8_t M = 0;
    int8_t D = 0;
    int8_t MO = 0;
    int16_t Y = 0;
  } S;

  HTTP.arg("T").toCharArray(TempBuf, 10);
  S.H = atoi(strtok(TempBuf, ":"));
  S.M = atoi(strtok(NULL, ":"));

  HTTP.arg("D").toCharArray(TempBuf, 15);
  S.Y = atoi(strtok(TempBuf, "-"));
  S.MO = atoi(strtok(NULL, "-"));
  S.D = atoi(strtok(NULL, "-"));

  Clock.hour = S.H;
  Clock.minute = S.M;
  Clock.year = S.Y;
  Clock.month = S.MO;
  Clock.date = S.D;

  Serial.println(msg);
  sprintf(msg, "Time: %d : %d", S.H, S.M);
  Serial.println(msg);
  sprintf(msg, "DataIN: %0004d.%02d.%02d", S.Y, S.MO, S.D);
  Serial.println(msg);
  sprintf(msg, "DataRTC: %0004d.%02d.%02d", Clock.year, Clock.month, Clock.date);
  Serial.println(msg);

  RTC.setTime(Clock);
  SaveConfig();

  Serial.println("Time Update");
  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// System settings
void SystemUpdate()
{
  sensors.T1_offset = HTTP.arg("T1O").toInt();
  sensors.T2_offset = HTTP.arg("T2O").toInt();
  CFG.num = HTTP.arg("N").toInt();

  // #ifndef DEBUG
  Serial.printf("T1_OFFset: %d \r\n", sensors.T1_offset);
  Serial.printf("T2_OFFset: %d \r\n", sensors.T2_offset);
  Serial.printf("BeehiveNum: %d \r\n", CFG.num);

  // #endif

  SaveConfig();
  // Show Led state (add function)
  Serial.println("System Update");
  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void SerialNumberUPD()
{
  CFG.sn = HTTP.arg("sn").toInt();
  Serial.printf("SN:");
  Serial.println(CFG.sn);
  SaveConfig();

  // EEP_Write();
  HTTP.send(200, "text/plain", "Serial Number set");
}
/*******************************************************************************************************/
/*******************************************************************************************************/
void HandleClient()
{
  HTTP.handleClient();
}
/*******************************************************************************************************/
/*******************************************************************************************************/
void SaveSecurity()
{
  char TempBuf[10];

  CFG.APSSID = HTTP.arg("ssid");
  CFG.APPAS = HTTP.arg("pass");

#ifndef DEBUG
  Serial.println("Network name:");
  Serial.print(CFG.APSSID);
  Serial.println();
  Serial.print("Network password:");
  Serial.println(CFG.APPAS);
#endif

  SaveConfig();
  // ShowLoadJSONConfig();

  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/
/*******************************************************************************************************/
// ESP Restart
void Restart()
{
  HTTP.send(200, "text/plain", "OK"); // Oтправляем ответ Reset OK
  Serial.println("Restart Core");
  ESP.restart(); // перезагружаем модуль
}
/*******************************************************************************************************/
/*******************************************************************************************************/
void FactoryReset()
{
  HTTP.send(200, "text/plain", "OK"); // Oтправляем ответ Reset OK
  Serial.println("#### FACTORY RESET ####");
  SystemFactoryReset();
  SaveConfig();
  // EEP_Write();
  // ShowFlashSave();
  Serial.println("#### SAVE DONE ####");
  ESP.restart(); // перезагружаем модуль
}
/*******************************************************************************************************/
void ShowSystemInfo()
{
  char msg[30];

  Serial.printf("System Information");
  sprintf(msg, "%s.%d", CFG.fw, CFG.sn);
  // SendXMLUserData(msg);

  HTTP.send(200, "text/plain", "OK"); // Oтправляем ответ Reset OK
}