#ifndef _Config_H
#define _Config_H

#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include "HardwareSerial.h"
#include <EEPROM.h>
#include <microDS3231.h>
#include <Wire.h>
#include <GyverBME280.h>
#include <EncButton.h>
#include <GyverOLED.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "HX711.h"

#define DEBUG
// #define I2C_SCAN

#define EB_DEB_TIME 20
#define EB_CLICK_TIME 50
#define EB_HOLD_TIME 600
#define EB_STEP_TIME 200

#define UARTSpeed 115200
#define MODEMSpeed 9600

#define WiFi_
#define WiFiTimeON 15
#define Client 0
#define AccessPoint 1
#define WorkNET

#define DISABLE 0
#define ENABLE 1

#define BAT_MIN 30

// I2C Adress
#define BME_ADR 0x76
#define OLED_ADR 0x3C
#define RTC_ADR 0x68

// GPIO PINs
#define SET_PIN 18 // кнопкa Выбор
#define PL_PIN 19  // кнопкa Плюс
#define MN_PIN 5   // кнопкa Минус

#define DS_SNS 4  // ds18b20
#define BAT 34    // Аккумулятор
#define TX_PIN 17 // SIM800_TX
#define RX_PIN 16 // SIM800_RX
#define HX_DT 25  // HX711_DT
#define HX_CLK 26 // HX711_CLK

//=======================================================================
extern MicroDS3231 RTC;
extern DateTime Clock;
//=======================================================================

//========================== ENUMERATION ================================
//=======================================================================
#define ITEMS 4 // Main Menu Items (Menu and Action is not includet)
enum menu
{
  Menu = 1,
  Action,
  Time,
  Notifycation,
  SMS_NUM,
  ZeroSet
};

enum loading
{
  _Logo = 0,
  _RTC,
  _SPIFSS,
  _Scale,
  _Sensors,
  _Modem,
  _WiFi_IP
};

//=======================================================================

//=========================== GLOBAL CONFIG =============================
struct GlobalConfig
{
  // General settings
  uint16_t sn = 0;
  String fw = ""; // accepts from setup()
  String fwdate = "17.11.2024";
  String chipID = "";
  String MacAdr = "";

  // WiFi Data settings
  String APSSID = "Beehive";
  String APPAS = "retra777zxc";

  String Ssid = "MkT";           // SSID Wifi network
  String Password = "QFCxfXMA3"; // Passwords WiFi network
  byte WiFiMode = AccessPoint;   // Режим работы WiFi
  byte IP1 = 192;
  byte IP2 = 168;
  byte IP3 = 1;
  byte IP4 = 1;
  byte GW1 = 192;
  byte GW2 = 168;
  byte GW3 = 1;
  byte GW4 = 1;
  byte MK1 = 255;
  byte MK2 = 255;
  byte MK3 = 255;
  byte MK4 = 0;

  // Beehive data
  uint8_t num = 1; // Beehive number
  // uint8_t st_cal = 0;
  float cal_f = 0.0; // Calibration factor
  int32_t avr = 0;   // Calibration average
  // int8_t t1_sms = 0;    // Time Point for send SMS_1
  // int8_t t2_sms = 0;
  // int8_t num[10] = {0};
  String phone = "";  // Phone Number (String)
  char phoneChar[12]; // Phone Number (to Char)
  int phoneInt[11] = {0};
  uint16_t iso_code = 7;
  int8_t UserSendTime1 = 0; // Time Point for send SMS_1
  int8_t UserSendTime2 = 0; // Time Point for send SMS_2
};
extern GlobalConfig CFG;
//=======================================================================

//=======================================================================
struct SYTM
{
  bool DispState = true;
  uint8_t DispMenu = Action;
  uint16_t tmrSec = 0;
  uint16_t tmrMin = 0;
};
extern SYTM System;
//=======================================================================

struct SNS
{
  int signal = 0;
  float dsT = 0.0;      // Temperature DS18B20
  int8_t T1_offset = 0; // Temperature Offset T1 sensor
  int8_t T2_offset = 0; // Temperature Offset T2 sensor
  float bmeT = 0.0;     // Temperature BME280
  int bmeH = 0.0;       // Humidity   BME280
  float bmeHcal = 4.2;  // Calibration factors
  float bmeA = 0.0;     // Altitude   BME280 m
  float bmeP_hPa = 0;   // Pressure   BME280 hPa
  int bmeP_mmHg = 0;    // Pressure   BME280 mmHg
  float calib = 0.0;    // Save and Reading from EEPROM
  float units = 0.0;
  float kg = 0.0;
  float grms = 10.5;
  int32_t averange = 0;
  uint32_t voltage = 0;
};
extern SNS sensors;
//=======================================================================

//=======================================================================
struct Flag
{
  bool WiFiEnable : 1;
  bool debug = true;
  bool SMS1 = true;
  bool SMS2 = true;
  bool HX711_Block = false;
  bool SetZero = false;
  bool Call_Block = false;
};
extern Flag ST;
//============================================================================

//============================================================================
void SystemInit(void); //  System Initialisation (variables and structure)
boolean SerialNumConfig(void);
void ShowInfoDevice(void); //  Show information or this Device
void GetChipID(void);
void CheckSystemState(void);
void DebugControl(void);
void SystemFactoryReset(void);
void I2C_Scanning(void);
//============================================================================
#endif // _Config_