#ifndef HTTP_H
#define HTTP_H

#include "Config.h"
#include "WF.h"
#include "FileConfig.h"
#include <WebServer.h>


void HTTPinit();
bool handleFileRead(String path);
String getContentType(String filename);
void UpdateData(void);
void UpdateState(void);
void NotificationUPD(void);
void ScaleCalSave(void);
void ScaleSetZero(void);
void TimeUpdate(void);
void SystemUpdate(void);
void WCLogiqUPD(void);
void SerialNumberUPD(void);
void SaveSecurity(void);
void HandleClient(void);
void Restart(void);
void FactoryReset(void);
void ShowSystemInfo(void);

#endif