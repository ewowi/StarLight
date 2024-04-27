/*
   @title     StarBase
   @file      SysModNetwork.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

#include <DNSServer.h>

class SysModNetwork:public SysModule {

public:

  SysModNetwork();

  //setup wifi an async webserver
  void setup();

  void loop1s();

  void handleConnection();
  void initConnection();

  void handleAP();
  void initAP();
  void stopAP();
  
private:
  bool apActive = false;
  unsigned32 lastReconnectAttempt = 0;
  byte apChannel = 1; // 2.4GHz WiFi AP channel (1-13)
  bool isConfirmedConnection = false;
  DNSServer dnsServer;
  byte stacO = 0; //stationCount
};
  
extern SysModNetwork *net;