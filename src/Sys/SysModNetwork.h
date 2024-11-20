/*
   @title     StarBase
   @file      SysModNetwork.h
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"

#include <DNSServer.h>

class SysModNetwork:public SysModule {

public:

  SysModNetwork();

  //setup wifi an async webserver
  void setup() override;

  void loop1s() override;
  void loop10s() override;

  // void handleConnections();

  #ifdef STARBASE_ETHERNET
    bool initEthernet();
    void stopEthernet();
  #endif

  void initWiFiConnection();
  void stopWiFiConnection();

  void initAP();
  void handleAP();
  void stopAP();

  IPAddress localIP();

private:
  #ifdef STARBASE_ETHERNET
    bool ethActive = false; //currently only one call to ETH.begin is possible, wether successful or not: reboot needed for other attempt
  #endif
  bool apActive = false;
  bool wfActive = false;
  byte apChannel = 1; // 2.4GHz WiFi AP channel (1-13)
  DNSServer dnsServer;
  byte stationCount = 0;

  bool connected = false;
};
  
extern SysModNetwork *net;