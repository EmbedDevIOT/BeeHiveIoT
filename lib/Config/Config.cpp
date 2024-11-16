#include "Config.h"
//=======================================================================
//=========================================================================
boolean SerialNumConfig()
{
  bool sn = false;
  if (CFG.sn == 0)
  {
    Serial.println("Serial number: FALL");
    Serial.println("Please enter SN");
    sn = true;
  }

  while (sn)
  {
    if (Serial.available())
    {
      // digitalWrite(LED_ST, HIGH);
      String r = Serial.readString();
      bool block_st = false;
      r.trim();
      CFG.sn = r.toInt();

      // digitalWrite(LED_ST, LOW);

      if (CFG.sn >= 1)
      {
        Serial.printf("Serial number: %d DONE \r\n", CFG.sn);
        sn = false;
        return true;
      }

      log_i("free heap=%i", ESP.getFreeHeap());
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
  return false;
}
//=========================================================================

/************************ System Initialisation **********************/
void SystemInit(void)
{
  GetChipID();
}
/*******************************************************************************************************/
//=======================   I2C Scanner     =============================
void I2C_Scanning(void)
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 8; address < 127; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}
//=======================================================================
/***************************** Function Show information or Device *************************************/
void ShowInfoDevice(void)
{
  Serial.println(F("Starting..."));
  Serial.println(F("Beehive"));
  Serial.print(F("SN:"));
  Serial.println(CFG.sn);
  Serial.print(F("fw_date:"));
  Serial.println(CFG.fwdate);
  Serial.println(CFG.fw);
  Serial.println(CFG.chipID);
  Serial.println(F("by EmbedDev"));
  Serial.println();
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void GetChipID()
{
  uint32_t chipId = 0;

  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  CFG.chipID = chipId;
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// String GetMacAdr()
// {
//   // Config.MacAdr = WiFi.macAddress(); //
//   // Serial.print(F("MAC:"));           // временно
//   // Serial.println(Config.MacAdr);     // временно
//   // return WiFi.macAddress();
// }
/*******************************************************************************************************/

/*******************************************************************************************************/
void CheckSystemState()
{
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// Debug information
void DebugControl()
{
  char message[50];

  Serial.println(F("!!!!!!!!!!!!!!  DEBUG INFO  !!!!!!!!!!!!!!!!!!"));
  // sprintf(message, "DISP:%d | ML %d | P: %d T: %02d:%02d ", System.DispState, System.DispMenu, disp_ptr, tmrMin, tmrSec);
  // Serial.println(message);
  sprintf(message, "RTC Time: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
  Serial.println(message);
  sprintf(message, "RTC Date: %4d.%02d.%02d", Clock.year, Clock.month, Clock.date);
  Serial.println(message);
  sprintf(message, "T_DS:%0.2f *C", sensors.dsT);
  Serial.println(message);
  sprintf(message, "T_BME:%0.2f *C | H_BME:%0d % | P_BHE:%d", sensors.bmeT, (int)sensors.bmeH, (int)sensors.bmeP_mmHg);
  Serial.println(message);
  sprintf(message, "WEIGHT: %0.2fg | CAL: %0.5f  | W_AVR: %0d", sensors.kg, sensors.calib, sensors.averange);
  Serial.println(message);
  sprintf(message, "BAT: %003d", sensors.voltage);
  Serial.println(message);
  sprintf(message, "SIM800 Signal: %d", sensors.signal);
  Serial.println(message);
  sprintf(message, "SMS_1 %02d | SMS_2 %02d", CFG.UserSendTime1, CFG.UserSendTime2);
  Serial.println(message);

  sprintf(message, "Phone: %s", CFG.phone);
  Serial.println(message);
  // sprintf(message, "Block Timer: %d", block_timer);
  // Serial.println(message);

  Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
  Serial.println();
}

/*******************************************************************************************************/

/*******************************************************************************************************/
void SystemFactoryReset()
{
}
/*******************************************************************************************************/
