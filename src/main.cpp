#include "Config.h"

#include "WF.h"
#include "HTTP.h"
#include "sim800.h"

//=======================================================================

//========================== DEFINITIONS ================================
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */

#define DISP_TIME (tmrMin == 10 && tmrSec == 0)

//=======================================================================

//============================== STRUCTURES =============================
// GlobalConfig Config;
GlobalConfig CFG;
SNS sensors;
SYTM System;
DateTime Clock;
Flag ST;
// EEP_D _eep;
//=======================================================================

//============================ GLOBAL VARIABLES =========================
uint8_t tim_sec = 0;
uint32_t now;

uint32_t block_timer = 0;

uint16_t tmrSec = 0;
uint16_t tmrMin = 0;
uint8_t disp_ptr = 0;
bool st = false; // menu state ()selection
bool i2c_wakeup = false;

char charPhoneNumber[11];

RTC_DATA_ATTR int bootCount = 0;
//================================ OBJECTs =============================
#define OLED_SOFT_BUFFER_64 // MCU buffer
GyverOLED<SSD1306_128x64> disp;

HX711 scale;
MicroDS3231 RTC; // 0x68
GyverBME280 bme; // 0x76

Button btUP(PL_PIN, INPUT_PULLUP);
Button btSET(SET_PIN, INPUT_PULLUP);
Button btDWN(MN_PIN, INPUT_PULLUP);
VirtButton btVirt;

// Dallas Themperature sensor DS18b20
OneWire oneWire(DS_SNS);
DallasTemperature ds18b20(&oneWire);

// Freertos Create Task object
TaskHandle_t Task0; // Task pinned to Core 0
TaskHandle_t Task1; // Task pinned to Core 0
TaskHandle_t Task2; // Task pinned to Core 1 (every 500 ms)
TaskHandle_t Task3; // Task pinned to Core 1 (every 1000 ms)
TaskHandle_t Task4; // Task pinned to Core 0 (every 5000 ms)
// FreeRTOS create Mutex link
SemaphoreHandle_t call_mutex;
//=======================================================================

//================================ PROTOTIPs =============================
void OledShowState(uint8_t state);
void ButtonHandler(void);
void DisplayHandler(uint8_t item);
void printPointer(uint8_t pointer);
void GetBatVoltage(void);
void GetDSData(void);
void GetBMEData(void);
void GetWeight(void);
void ShowDBG(void);
void Notification(void);
void SetZero(void);
void TaskCore0(void *pvParameters);
void TaskCore1(void *pvParameters);
void Task500ms(void *pvParameters);
void Task1000ms(void *pvParameters);
void Task5s(void *pvParameters);
void I2CwakeUP();
//=======================================================================

//=======================================================================
// Core 0
void TaskCore0(void *pvParameters)
{
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    if (!ST.Call_Block)
    {
      if (!ST.HX711_Block)
        GetWeight();

      if (ST.SetZero)
      {
        SetZero();
      }
    }
    vTaskDelay(500 / portTICK_RATE_MS);
  }
}

// Core 0
void TaskCore1(void *pvParameters)
{
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;)
  {
    if (block_timer != 20)
    {
      if (!ST.Call_Block)
        ButtonHandler();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Core 1 | Every 500ms Read RTC Data and Notification
void Task500ms(void *pvParameters)
{
  Serial.print("Task500ms running on core ");
  Serial.println(xPortGetCoreID());
  while (true)
  {
    if (block_timer != 20)
    {
      // xSemaphoreTake(call_mutex, portMAX_DELAY);
      // IncommingRing();
      Notification();
      Clock = RTC.getTime();
      // xSemaphoreTake(call_mutex, portMAX_DELAY);
    }

    vTaskDelay(500 / portTICK_RATE_MS);
  }
}

// Task every 1000ms (Get Voltage and Show Debug info)
void Task1000ms(void *pvParameters)
{
  Serial.print("Task1000 running on core ");
  Serial.println(xPortGetCoreID());
  while (1)
  {
    if (block_timer != 20)
    {
      // 1 Min Timer
      if (tim_sec < 59)
        tim_sec++;
      else
      {
        tim_sec = 0;
        block_timer++;
        if (!ST.Call_Block)
        {
          GetLevel();
        }
      }

      if (System.DispState)
      {
        DisplayHandler(System.DispMenu);

        if (tmrSec < 59)
        {
          tmrSec++;
        }
        else
        {
          tmrSec = 0;
          tmrMin++;
        }
      }
      else
        disp.setPower(false);

      if DISP_TIME
      {
        System.DispState = false;
        ST.WiFiEnable = false;
        Serial.println("WiFi_Disable");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        Serial.println("TimeOut: Display - OFF");
        tmrMin = 0;
        tmrSec = 0;
        disp_ptr = 0;
      }

#ifdef DEBUG
      if (!ST.Call_Block)
      {
        // xSemaphoreTake(uart_mutex, portMAX_DELAY);
        // ShowDBG();
        DebugControl();
        // xSemaphoreGive(uart_mutex);
      }
#endif
    }
    else
    {
      disp.setPower(false);
      Serial.println("TimeOut: Trial Version Tim block: ");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void Task5s(void *pvParametrs)
{
  while (1)
  {
    if (!ST.Call_Block)
    {
      GetBatVoltage();
      GetBMEData();
      GetDSData();
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

//=======================================================================
void OledShowState(uint8_t state)
{
  char msg[32];

  switch (state)
  {
  case _Logo:
    disp.clear();
    disp.setScale(2); // масштаб текста (1..4)
    disp.setCursor(20, 3);
    sprintf(msg, "Beehive");
    disp.print(msg);
    Serial.println(msg);

    disp.setScale(1);
    disp.setCursor(20, 7);
    sprintf(msg, "firmware:%s", CFG.fw);
    disp.print(msg);
    Serial.println(msg);

    disp.update();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    disp.clear();
    break;
  case _RTC:
    disp.setScale(1);
    disp.setCursor(1, 1);
    sprintf(msg, "RTC");
    disp.print(msg);
    disp.setCursor(110, 1);
    sprintf(msg, "OK");
    disp.print(msg);
    disp.update();
    break;
  case _SPIFSS:
    disp.setScale(1);
    disp.setCursor(1, 2);
    sprintf(msg, "FS");
    disp.print(msg);
    disp.setCursor(110, 2);
    sprintf(msg, "OK");
    disp.print(msg);
    disp.update();
    break;
  case _Scale:
    disp.setScale(1);
    disp.setCursor(1, 3);
    sprintf(msg, "Scale");
    disp.print(msg);
    disp.setCursor(110, 3);
    sprintf(msg, "OK");
    disp.print(msg);
    disp.update();
    break;
  case _Sensors:
    disp.setScale(1);
    disp.setCursor(1, 4);
    sprintf(msg, "Sensors");
    disp.print(msg);
    disp.setCursor(110, 4);
    sprintf(msg, "OK");
    disp.print(msg);
    disp.update();
    break;
  case _Modem:
    disp.setScale(1);
    disp.setCursor(1, 5);
    sprintf(msg, "Modem");
    disp.print(msg);
    disp.setCursor(110, 5);
    sprintf(msg, "OK");
    disp.print(msg);
    disp.update();
    break;
  case _WiFi_IP:
    disp.clear();
    disp.setScale(2);
    disp.setCursor(45, 1);
    sprintf(msg, "WiFi");
    disp.print(msg);
    Serial.println(msg);

    disp.setScale(2);
    disp.setCursor(1, 5);
    sprintf(msg, "%d.%d.%d.%d", CFG.IP1, CFG.IP2, CFG.IP3, CFG.IP4);
    disp.print(msg);
    Serial.println(msg);

    disp.update();
    vTaskDelay(2500 / portTICK_PERIOD_MS);
    disp.clear();
    break;
  default:
    break;
  }
}
//=======================================================================

//=======================       SETUP     ===============================
void setup()
{
  CFG.fw = "1.0.0";
  CFG.fwdate = "15.11.24";
  // UART Init
  Serial.begin(UARTSpeed);
  Serial1.begin(MODEMSpeed);
  // OLED INIT
  Wire.begin();
  disp.init();
  disp.setContrast(255);
  disp.clear();
  Serial.println(F("OLED...Done"));
  // Show starting info
  OledShowState(_Logo);
  // RTC INIT
  RTC.begin();
  // RTC battery crash
  if (RTC.lostPower())
  {
    RTC.setTime(COMPILE_TIME);
  }
  Clock = RTC.getTime();
  OledShowState(_RTC);
  Serial.println(F("RTC...Done"));

  // SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  else
  {
    LoadConfig();          // Load configuration from config.json files
    ShowLoadJSONConfig();  // Show load configuration
    if (SerialNumConfig()) // Set Serial Number
      SaveConfig();
    OledShowState(_SPIFSS);
  }

  // // HX711 Init
  scale.begin(HX_DT, HX_CLK);
  scale.set_scale();
  scale.set_offset(sensors.averange);
  OledShowState(_Scale);
  Serial.println(F("HX711 init done"));

  // BME and DS SENSOR INIT
  bme.begin(0x76);
  Serial.println(F("BME...Done"));
  ds18b20.begin();
  Serial.println(F("DS18b20...Done"));
  vTaskDelay(20 / portTICK_PERIOD_MS);
  // Battery pin init
  pinMode(BAT, INPUT);
  OledShowState(_Sensors);
  Serial.println(F("Battery Init...Done"));
  // SIM800 INIT
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sim800_init(9600, 16, 17);
  sim800_conf();
  OledShowState(_Modem);
  Serial.println(F("SIM800 Init...Done"));
  disp.clear();

  GetBatVoltage();
  GetBMEData();
  GetDSData();
  GetWeight();
  GetLevel();

  Serial.println("Wifi init");
  WIFIinit();
  vTaskDelay(500 / portTICK_PERIOD_MS);
  Serial.println("Wifi Enable");
  OledShowState(_WiFi_IP);

  HTTPinit(); // HTTP server initialisation
  vTaskDelay(500 / portTICK_PERIOD_MS);

  disp.update();

  call_mutex = xSemaphoreCreateMutex();
  // Create Task. Running to core 0
  xTaskCreatePinnedToCore(
      TaskCore0, // Функция для задачи
      "Task0",   // Имя задачи
      10000,     // Размер стека
      NULL,      // Параметр задачи
      1,         // Приоритет
      &Task0,    // Выполняемая операция
      0          // Номер ядра
  );
  delay(100);

  // Create Task. Running to core 1
  xTaskCreatePinnedToCore(
      TaskCore1,
      "Task1",
      12228,
      NULL,
      1,
      &Task1,
      0);
  delay(100);

  xTaskCreatePinnedToCore(
      Task500ms,
      "Task2",
      2048,
      NULL,
      1,
      &Task2,
      1);
  delay(100);

  xTaskCreatePinnedToCore(
      Task1000ms,
      "Task3",
      4096,
      NULL,
      1,
      &Task3,
      1);

  xTaskCreatePinnedToCore(
      Task5s,
      "Task4",
      2048,
      NULL,
      1,
      &Task4,
      0);

  Serial.println(F("FreeRTOS started...Done"));
}

//=========================      M A I N       ===========================
void loop()
{
  // HTTP Handling else WiFi = ON (Counter 10 min)
  IncommingRing();
  if (ST.WiFiEnable)
  {
    HandleClient();
  }
}
//========================================================================

//========================================================================
void Notification()
{
  char buf[128] = "";
  if (ST.SMS1 && Clock.hour == CFG.UserSendTime1 && Clock.minute == 30 && Clock.second == 0)
  {
    Serial.println("Send Notification: SMS1");
    ST.Call_Block = true;
    SendUserSMS();

    delay(200);

    ST.SMS1 = false;
    ST.SMS2 = true;
    ST.Call_Block = false;
  }

  if (ST.SMS2 && Clock.hour == CFG.UserSendTime2 && Clock.minute == 0 && Clock.second == 0)
  {
    Serial.println("Send Notification: SMS2");
    ST.Call_Block = true;

    SendUserSMS();

    delay(200);
    ST.SMS1 = true;
    ST.SMS2 = false;
    ST.Call_Block = false;
  }
}
//========================================================================

//========================================================================
void ButtonHandler()
{
  btSET.tick();
  btUP.tick();
  btDWN.tick();
  btVirt.tick(btUP, btDWN);
  // Button SET
  if (btSET.click())
  {
    Serial.println("Btn SET click");
    if (System.DispMenu == Action)
    {
      System.DispMenu = Menu;
    }

    if (System.DispState == true)
    {
      if (System.DispMenu == Menu && disp_ptr == 0)
      {
        if (!st)
        {
          System.DispMenu = Menu;
          Serial.println("General Menu:");
          disp.clear();
          disp.home();
          disp.setScale(1);
          disp.print(F(
              "  Время:\r\n"
              // "  Калибровка:\r\n"
              "  Оповещения:\r\n"
              // "  Аккумулятор:\r\n"
              "  Номер СМС:\r\n"
              "  Выход:\r\n"));

          printPointer(disp_ptr); // Show pointer
          disp.update();
          st = true;
        }
        else
        {
          System.DispMenu = Time;
          Serial.println("Time Menu:");
          st = false;
        }
      }

      // if (System.DispMenu == Menu && disp_ptr == 1)
      // {
      //   System.DispMenu = Calib;
      //   Serial.println("Calibration Menu:");
      // }

      if (System.DispMenu == Menu && disp_ptr == 1)
      {
        System.DispMenu = Notifycation;
        Serial.println("Notifycation menu:");
      }

      if (System.DispMenu == Menu && disp_ptr == 2)
      {
        System.DispMenu = SMS_NUM;
        Serial.println("SMS Number:");
      }

      if (System.DispMenu == Menu && disp_ptr == 3)
      {
        Serial.println("Exit");

        System.DispMenu = Action;
        disp_ptr = 0;
        st = false;
      }
    }
    else // enable display and enable WiFi
    {
      disp.setPower(true);
      System.DispMenu = Action;
      System.DispState = true;

      ST.WiFiEnable = true;
      Serial.println("WiFi_Enable");
      WIFIinit(AccessPoint);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      HTTPinit(); // HTTP server initialisation
    }

    tmrMin = 0;
    tmrSec = 0;
    disp.clear();
  }
  // Button UP
  if (btUP.click() || btUP.hold())
  {
    Serial.println("Btn UP click");
    tmrMin = 0;
    tmrSec = 0;

    if (System.DispMenu == Menu)
      disp_ptr = constrain(disp_ptr + 1, 0, ITEMS - 1);

    sensors.calib += 0.01;

    Serial.printf("Save CAL:%0.4f \r\n", sensors.calib);
    SaveConfig();

    // Serial.printf("ptr:%d \r\n", disp_ptr);
    Serial.println();
  }
  // UP + DOWN
  if (btVirt.click() && System.DispMenu == Action)
  {
    Serial.println("Set zero");
    ST.SetZero = true;
    System.DispMenu = ZeroSet;
  }
  // DOWN
  if (btDWN.click() || btDWN.hold())
  {
    Serial.println("Btn DWN click");
    tmrMin = 0;
    tmrSec = 0;

    if (System.DispMenu == Menu)
      disp_ptr = constrain(disp_ptr - 1, 0, ITEMS - 1);

    sensors.calib -= 0.01;
    Serial.printf("Save CAL:%0.4f \r\n", sensors.calib);
    SaveConfig();

    // Serial.printf("ptr:%d \r\n", disp_ptr);
  }
}
//========================================================================

//========================================================================
// Get Data from BME Sensor
void GetBMEData()
{
  sensors.bmeT = bme.readTemperature() + sensors.T2_offset;
  sensors.bmeH = (int)bme.readHumidity() + sensors.bmeHcal;
  sensors.bmeP_hPa = bme.readPressure();
  sensors.bmeP_mmHg = (int)pressureToMmHg(sensors.bmeP_hPa);
}
//========================================================================

//========================================================================
// Get Data from DS18B20 Sensor
void GetDSData()
{
  ds18b20.requestTemperatures();
  sensors.dsT = ds18b20.getTempCByIndex(0);
  sensors.dsT = sensors.dsT + sensors.T1_offset;
}
//========================================================================

//========================================================================
// Working with HX711
// Get data from HX711 (Every 500 ms)
void GetWeight()
{
  scale.set_scale(sensors.calib);
  sensors.units = scale.get_units(5);
  sensors.grms = (sensors.units * 0.035274);
  sensors.kg = float(sensors.grms / 1000);
  if (sensors.kg < 0)
    sensors.kg = 0.0;
  sensors.kg = constrain(sensors.kg, 0.0, 200.0);
}

// Scale HX711 Zeroing (request from HTTP servers)
void SetZero()
{
  ST.HX711_Block = true;
  Serial.println("HX711 Zeroing");
  sensors.averange = scale.read_average(5);
  Serial.printf("Averange: %d \r\n", sensors.averange);
  scale.set_offset(sensors.averange);
  ST.HX711_Block = false;
  ST.SetZero = false;
  vTaskDelay(500 / portTICK_PERIOD_MS);
}
//========================================================================

//========================================================================
void DisplayHandler(uint8_t item)
{
  switch (item)
  {
    char dispbuf[30];
  case Menu:
  {
    disp.clear();
    disp.home();
    disp.setScale(1);
    disp.print(F(
        "  Время:\r\n"
        // "  Калибровка:\r\n"
        "  Оповещения:\r\n"
        "  Номер СМС:\r\n"
        "  Выход:\r\n"));

    printPointer(disp_ptr); // Show pointer
    disp.update();
    break;
  }

  case Action:
  {
    sprintf(dispbuf, "%02d:%02d", Clock.hour, Clock.minute);
    disp.setScale(2);
    disp.setCursor(0, 0);
    disp.print(dispbuf);

    disp.setCursor(85, 0);
    if (sensors.voltage == 0)
    {
      sprintf(dispbuf, "---");
    }
    else
      sprintf(dispbuf, "%3d", sensors.voltage);
    disp.print(dispbuf);

    sprintf(dispbuf, "%0.1f", sensors.kg);
    disp.setScale(3);
    disp.setCursor(40, 2);
    disp.print(dispbuf);

    sprintf(dispbuf, "T1:%0.1fC     T2:%0.1fC", sensors.dsT, sensors.bmeT);
    disp.setScale(1);
    disp.setCursor(0, 6);
    disp.print(dispbuf);

    sprintf(dispbuf, "H:%02d            P:%003d", sensors.bmeH, sensors.bmeP_mmHg);
    disp.setCursor(0, 7);
    disp.print(dispbuf);
    disp.update();
    break;
  }

  case Time:
  {
    int8_t _hour = RTC.getHours();
    int8_t _min = RTC.getMinutes();
    bool set = false;

    disp.clear();
    disp.setScale(2);
    disp.setCursor(0, 0);
    disp.print(F(" Установка \r\n"
                 "  времени \r\n"));
    disp.setCursor(35, 5);
    sprintf(dispbuf, "%02d:", _hour);
    disp.print(dispbuf);

    disp.invertText(true);
    sprintf(dispbuf, "%02d", _min);
    disp.print(dispbuf);
    disp.update();

    while (!set)
    {
      bool _setH = false;
      bool _setM = true;

      btSET.tick();
      btUP.tick();
      btDWN.tick();

      // Setting Minute
      while (_setM)
      {
        btSET.tick();
        btUP.tick();
        btDWN.tick();

        if (btUP.click())
        {
          disp.clear();
          disp.setScale(2);
          disp.setCursor(0, 0);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          _min++;
          if (_min > 59)
            _min = 0;

          disp.setCursor(35, 5);
          sprintf(dispbuf, "%02d:", _hour);
          disp.print(dispbuf);

          disp.invertText(true);
          sprintf(dispbuf, "%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }

        if (btDWN.click())
        {
          disp.clear();
          disp.setCursor(0, 0);
          disp.setScale(2);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          _min--;
          if (_min < 0)
            _min = 59;

          disp.setCursor(35, 5);
          sprintf(dispbuf, "%02d:", _hour);
          disp.print(dispbuf);

          disp.invertText(true);
          sprintf(dispbuf, "%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }
        // Exit Set MIN and select Hour set
        if (btSET.click())
        {
          _setM = false;
          _setH = true;
          Serial.println(F("Minute set"));

          disp.clear();
          disp.setScale(2);
          disp.setCursor(0, 0);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          disp.setCursor(35, 5);
          disp.invertText(true);
          sprintf(dispbuf, "%02d", _hour);
          disp.print(dispbuf);

          disp.invertText(false);
          sprintf(dispbuf, ":%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }
      }
      //  HOUR
      while (_setH)
      {
        btSET.tick();
        btUP.tick();
        btDWN.tick();

        if (btUP.click())
        {
          disp.clear();
          disp.setCursor(0, 0);
          disp.setScale(2);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          _hour++;
          if (_hour > 23)
            _hour = 0;

          disp.invertText(true);
          disp.setCursor(35, 5);
          sprintf(dispbuf, "%02d", _hour);
          disp.print(dispbuf);

          disp.invertText(false);
          sprintf(dispbuf, ":%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }

        if (btDWN.click())
        {
          disp.clear();
          disp.setCursor(0, 0);
          disp.setScale(2);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          _hour--;
          if (_hour < 0)
            _hour = 23;

          disp.invertText(true);
          disp.setCursor(35, 5);
          sprintf(dispbuf, "%02d", _hour);
          disp.print(dispbuf);

          disp.invertText(false);
          sprintf(dispbuf, ":%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }
        // Exit Set HOUR and SAVE settings
        if (btSET.click())
        {
          _setM = false; // flag set Min (need to exit)
          _setH = false; // flag set Hour(need to exit)
          set = true;    // flag set Time (need to exit)
          Serial.println(F("HOUR set"));
          RTC.setTime(0, _min, _hour, Clock.date, Clock.month, Clock.year);

          st = false;
          System.DispMenu = Action;
          disp_ptr = 0;

          disp.clear();
          disp.invertText(false);
          disp.setScale(2);
          disp.setCursor(13, 3);
          disp.print("Сохранено");
          disp.update();
          delay(500);
          disp.clear();
        }
      }
    }
    break;
  }

    // case Calib:
    // {
    //   disp.clear();
    //   disp.setScale(2);
    //   disp.setCursor(0, 0);
    //   disp.print("Калибровка");
    //   disp.setCursor(17, 5);
    //   disp.printf("  %0.2f  ", sensors.kg);
    //   disp.update();
    //   // ST.HX711_Block = true;

    //   while (1)
    //   {
    //     btSET.tick();
    //     btUP.tick();
    //     btDWN.tick();

    //     while (btUP.busy())
    //     {
    //       btUP.tick();

    //       if (btUP.click())
    //       {
    //         sensors.calib += 0.01;

    //         Serial.printf("C:%0.4f \r\n", sensors.calib);
    //         GetWeight();
    //         Serial.printf("W:%0.2f \r\n", sensors.kg);

    //         disp.clear();
    //         disp.setScale(2);
    //         disp.setCursor(0, 0);
    //         disp.print("Калибровка");
    //         disp.setCursor(17, 5);
    //         disp.printf("  %0.2f  ", sensors.kg);
    //         disp.update();
    //       }

    //       if (btUP.step())
    //       {
    //         sensors.calib += 0.1;

    //         Serial.printf("C:%0.4f \r\n", sensors.calib);
    //         GetWeight();
    //         Serial.printf("W:%0.2f \r\n", sensors.kg);

    //         disp.clear();
    //         disp.setScale(2);
    //         disp.setCursor(0, 0);
    //         disp.print("Калибровка");
    //         disp.setCursor(17, 5);
    //         disp.printf("  %0.2f  ", sensors.kg);
    //         disp.update();
    //       }
    //     }

    //     while (btDWN.busy())
    //     {
    //       btDWN.tick();
    //       if (btDWN.click())
    //       {
    //         sensors.calib -= 0.01;

    //         Serial.printf("C: %0.4f \r\n", sensors.calib);
    //         GetWeight();
    //         Serial.printf("W: %0.2f \r\n", sensors.kg);

    //         disp.clear();
    //         disp.setScale(2);
    //         disp.setCursor(0, 0);
    //         disp.print("Калибровка");
    //         disp.setCursor(17, 5);
    //         disp.printf("  %0.2f  ", sensors.kg);
    //         disp.update();
    //       }

    //       if (btDWN.step())
    //       {
    //         sensors.calib -= 0.1;

    //         Serial.printf("C: %0.4f \r\n", sensors.calib);
    //         GetWeight();
    //         Serial.printf("W: %0.2f \r\n", sensors.kg);

    //         disp.clear();
    //         disp.setScale(2);
    //         disp.setCursor(0, 0);
    //         disp.print("Калибровка");
    //         disp.setCursor(17, 5);
    //         disp.printf("  %0.2f  ", sensors.kg);
    //         disp.update();
    //       }
    //     }

    //     // Exit Set CAlibration and SAVE settings
    //     if (btSET.click())
    //     {
    //       SaveConfig();
    //       Serial.println(F("Calibration SAVE"));

    //       System.DispMenu = Action;
    //       disp_ptr = 0;
    //       st = false;

    //       disp.clear();
    //       disp.setScale(2);
    //       disp.setCursor(13, 3);
    //       disp.print("Сохранено");
    //       disp.update();
    //       delay(500);
    //       disp.clear();
    //       // ST.HX711_Block = false;
    //       return;
    //     }
    //   }
    //   break;
    // }

  case Notifycation:
  {
    disp.clear();
    disp.invertText(false);
    disp.setScale(2);
    disp.setCursor(13, 0);
    disp.print("Время СМС");
    disp.setCursor(25, 3);
    disp.invertText(true);
    disp.printf("SMS1:%d", CFG.UserSendTime1);
    disp.invertText(false);
    disp.setCursor(25, 5);
    disp.printf("SMS2:%d", CFG.UserSendTime2);
    disp.update();

    bool _setSMS1 = true;
    bool _setSMS2 = false;
    // Set SMS_1
    while (_setSMS1)
    {
      btSET.tick();
      btUP.tick();
      btDWN.tick();

      if (btUP.click())
      {
        CFG.UserSendTime1++;

        if (CFG.UserSendTime1 > 23)
        {
          CFG.UserSendTime1 = 0;
        }

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.invertText(true);
        disp.printf("SMS1:%d", CFG.UserSendTime1);
        disp.setCursor(25, 5);
        disp.invertText(false);
        disp.printf("SMS2:%d", CFG.UserSendTime2);
        disp.update();
      }

      if (btDWN.click())
      {
        CFG.UserSendTime1--;

        if (CFG.UserSendTime1 < 0)
        {
          CFG.UserSendTime1 = 23;
        }

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.invertText(true);
        disp.printf("SMS1:%d", CFG.UserSendTime1);
        disp.setCursor(25, 5);
        disp.invertText(false);
        disp.printf("SMS2:%d", CFG.UserSendTime2);
        disp.update();
      }

      // Exit Set Calibration and SAVE settings
      if (btSET.click())
      {
        SaveConfig();

        Serial.println(F("SMS_1_MSG SAVE"));

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.invertText(false);
        disp.printf("SMS1:%d", CFG.UserSendTime1);
        disp.setCursor(25, 5);
        disp.invertText(true);
        disp.printf("SMS2:%d", CFG.UserSendTime2);
        disp.update();

        _setSMS1 = false; // flag to EXIT
        _setSMS2 = true;  // flag to enter in set SMS2
      }
    }
    // Set SMS_2
    while (_setSMS2)
    {
      btSET.tick();
      btUP.tick();
      btDWN.tick();

      if (btUP.click())
      {
        CFG.UserSendTime2++;

        if (CFG.UserSendTime2 > 23)
        {
          CFG.UserSendTime2 = 0;
        }

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.printf("SMS1:%d", CFG.UserSendTime1);
        disp.invertText(true);
        disp.setCursor(25, 5);
        disp.printf("SMS2:%d", CFG.UserSendTime2);
        disp.update();
      }

      if (btDWN.click())
      {
        CFG.UserSendTime2--;
        if (CFG.UserSendTime2 < 0)
        {
          CFG.UserSendTime2 = 23;
        }

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.printf("SMS1:%d", CFG.UserSendTime1);
        disp.invertText(true);
        disp.setCursor(25, 5);
        disp.printf("SMS2:%d", CFG.UserSendTime2);
        disp.update();
      }

      // Exit Set Calibration and SAVE settings
      if (btSET.click())
      {
        SaveConfig();

        Serial.println(F("SMS_2_MSG SAVE"));

        System.DispMenu = Action;
        disp_ptr = 0;
        st = false;

        disp.invertText(false);

        disp.clear();

        disp.clear();
        disp.setScale(2);
        disp.setCursor(13, 3);
        disp.print("Сохранено");
        disp.update();
        delay(500);
        disp.clear();
        _setSMS1 = false; // flag to EXIT
        _setSMS2 = false; // flag to EXIT
      }
    }
    break;
  }

  case SMS_NUM:
  {
    int currentDigit = 0;
    // Converting Phone Number (String to char)
    CFG.phone.toCharArray(CFG.phoneChar, 12);
    // Converting Phone Number (Char to Int array)
    Serial.printf("Phone Int:");

    for (int i = 0; i < 11; i++)
    {
      CFG.phoneInt[i] = (CFG.phoneChar[i] - '0');
    }

    disp.clear();
    disp.setScale(2);
    disp.setCursor(0, 0);
    disp.print("СМС Номер:");

    disp.setCursor(0, 5);
    for (int i = 0; i < 11; i++)
    {
      if (i == currentDigit)
      {
        disp.invertText(true);
      }
      else
        disp.invertText(false);
      disp.print(CFG.phoneInt[i]);
    }

    disp.update();

    while (currentDigit != 11)
    {
      btSET.tick();
      btUP.tick();
      btDWN.tick();

      if (btUP.click())
      {
        CFG.phoneInt[currentDigit] = (CFG.phoneInt[currentDigit] + 1) % 10;

        disp.clear();
        disp.setScale(2);
        disp.invertText(false);
        disp.setCursor(0, 0);
        disp.print("СМС Номер:");

        disp.setCursor(0, 5);
        for (int i = 0; i < 11; i++)
        {
          (i == currentDigit) ? disp.invertText(true) : disp.invertText(false);
          disp.print(CFG.phoneInt[i]);
        }
        disp.update();
      }

      if (btDWN.click())
      {
        CFG.phoneInt[currentDigit] = (CFG.phoneInt[currentDigit] - 1 + 10) % 10;

        disp.clear();
        disp.setScale(2);
        disp.invertText(false);
        disp.setCursor(0, 0);
        disp.print("СМС Номер:");

        disp.setCursor(0, 5);
        for (int i = 0; i < 11; i++)
        {
          (i == currentDigit) ? disp.invertText(true) : disp.invertText(false);
          disp.print(CFG.phoneInt[i]);
        }
        disp.update();
      }

      // Exit Set CAlibration and SAVE settings
      if (btSET.click())
      {
        currentDigit++;

        Serial.printf("Current Digit: %d", currentDigit);
        Serial.println();

        disp.clear();
        disp.setScale(2);
        disp.invertText(false);
        disp.setCursor(0, 0);
        disp.print("СМС Номер:");

        disp.setCursor(0, 5);
        for (int i = 0; i < 11; i++)
        {
          (i == currentDigit) ? disp.invertText(true) : disp.invertText(false);
          disp.print(CFG.phoneInt[i]);
        }
        disp.update();
      }
    }
    // Converting PhoneNumber (Int to Char )
    for (int i = 0; i < 11; i++)
    {
      CFG.phoneChar[i] = (char)(CFG.phoneInt[i] + '0');
    }

    CFG.phone.clear(); // Clear String
    // CFG.phone += '+';
    // CFG.phone += CFG.iso_code;
    CFG.phone += CFG.phoneChar;

    Serial.printf("Save SMS Number: %s", CFG.phone);
    Serial.println();
    SaveConfig();

    System.DispMenu = Action;
    disp_ptr = 0;
    st = false;

    disp.clear();
    disp.setScale(2);
    disp.setCursor(13, 3);
    disp.invertText(false);
    disp.print("Сохранено");
    disp.update();
    delay(500);
    disp.clear();
    break;
  }

  case ZeroSet:
  {
    char msg[50];
    ST.SetZero = true;

    disp.clear();
    disp.setScale(2);
    disp.setCursor(0, 1);
    disp.print(F(
        " Установка \r\n"
        "   нуля \r\n"
        " подождите  \r\n"));
    disp.update();

    st = false;
    System.DispMenu = Action;
    disp_ptr = 0;

    disp.clear();
    break;
  }

  default:
    break;
  }
}
//========================================================================

//========================================================================
void printPointer(uint8_t pointer)
{
  disp.setCursor(0, pointer);
  disp.print(">");
  // disp.update();
}
//========================================================================

//========================================================================
void GetBatVoltage(void)
{
  uint32_t _mv = 0;
  // 12.82 - 1195
  // 10.8  - 1010
  const uint16_t min = 1010, max = 1195;
  // 1250 = 10.8
  // 1380 = 12.82
  // const uint16_t min = 1250, max = 1390;

  for (uint8_t i = 0; i < 12; i++)
  {
    _mv += analogReadMilliVolts(BAT);
  }
  _mv = _mv / 12;

  // Serial.println(_mv);

  if (_mv == 0 || _mv < min)
  {
    _mv = 0;
  }
  else if (_mv >= min && _mv <= max)
  {
    _mv = map(_mv, min, max, 10, 100);
  }
  else if (_mv > max)
  {
    _mv = max;
    _mv = map(_mv, min, max, 10, 100);
  }
  sensors.voltage = _mv;
}
//========================================================================

/* Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void I2CwakeUP()
{
  Wire.begin();
  bme.begin(0x76);
  GetBMEData();
  RTC.begin();
}