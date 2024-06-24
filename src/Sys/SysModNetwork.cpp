/*
   @title     StarBase
   @file      SysModNetwork.cpp
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModNetwork.h"
#include "SysModules.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModUI.h"
#include "SysModModel.h"
#include "User/UserModMDNS.h"

SysModNetwork::SysModNetwork() :SysModule("Network") {};

//setup wifi an async webserver
void SysModNetwork::setup() {
  SysModule::setup();

  parentVar = ui->initSysMod(parentVar, name, 3502);
  parentVar["s"] = true; //setup

  // JsonObject tableVar = ui->initTable(parentVar, "wfTbl", nullptr, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { //varFun ro false: create and delete row possible
  //   ui->setLabel(var, "Wifi");
  //   ui->setComment(var, "List of defined and available Wifi APs");
  // });

  ui->initText(parentVar, "ssid", "", 31, false);

  ui->initPassword(parentVar, "pw", "", 63, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Password");
      return true;
    default: return false;
  }});

  ui->initButton(parentVar, "connect", false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    // case onUI:
    //   ui->setComment(var, "Force reconnect (loose current connection)");
    //   return true;
    case onChange:
      // mdl->doWriteModel = true; //saves the model
      initConnection(); //try to connect
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "nwstatus", nullptr, 32, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Status");
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "rssi", nullptr, 32, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Wifi signal");
      return true;
    default: return false;
  }});
}

void SysModNetwork::loop1s() {
  handleConnection(); //once per second is enough
  mdl->setUIValueV("rssi", "%d dBm", WiFi.RSSI());
}

void SysModNetwork::handleConnection() {
  if (lastReconnectAttempt == 0) { // do this only once
    ppf("lastReconnectAttempt == 0\n");
    initConnection();
    return;
  }

  if (apActive) {
    handleAP();
  }

  //if not connected to Wifi
  if (!(WiFi.localIP()[0] != 0 && WiFi.status() == WL_CONNECTED)) { //!Network.isConfirmedConnection()
    if (isConfirmedConnection) { //should not be confirmed as not connected -> lost connection -> retry
      ppf("Disconnected!\n");
      initConnection();
    }

    //if no connection for more then 6 seconds (was 12)
    if (!apActive && millis() - lastReconnectAttempt > 6000 ) { //&& (!wasConnected || apBehavior == AP_BEHAVIOR_NO_CONN)
      ppf("Not connected AP.\n");
      initAP();
    }
  } else if (!isConfirmedConnection) { //newly connected
    mdl->setUIValueV("nwstatus", "Connected %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    ppf("Connected %s\n", WiFi.localIP().toString().c_str());

    isConfirmedConnection = true;

    mdls->newConnection = true; // send all modules connect notification

    // shut down AP
    if (apActive) { //apBehavior != AP_BEHAVIOR_ALWAYS
      stopAP();
    }
  }
}

void SysModNetwork::initConnection() {

  WiFi.disconnect(true);        // close old connections

  lastReconnectAttempt = millis();

  //close ap if not ap
  if (!apActive) {
    stopAP();
    WiFi.mode(WIFI_STA);
  }

  const char * ssid = mdl->getValue("ssid");
  const char * password = mdl->getValue("pw");
  if (ssid && strlen(ssid)>0 && password) {
    char passXXX [64] = "";
    for (int i = 0; i < strlen(password); i++) strncat(passXXX, "*", sizeof(passXXX)-1);
    ppf("Connecting to WiFi %s / %s\n", ssid, passXXX);
    WiFi.begin(ssid, password);
    #if defined(STARBASE_LOLIN_WIFI_FIX )
      WiFi.setTxPower(WIFI_POWER_8_5dBm );
    #endif
    WiFi.setSleep(false);
    WiFi.setHostname(mdns->cmDNS); //use the mdns name (instance name or star-mac)
  }
  else
    ppf("initConnection error s:%s p:%s\n", ssid?ssid:"No SSID", password?password:"No Password");

  isConfirmedConnection = false; //need to test if really connected in handleConnection
}

void SysModNetwork::initAP() {
  const char * apSSID = mdl->getValue("name");
  ppf("Opening access point %s\n", apSSID);
  WiFi.softAPConfig(IPAddress(4, 3, 2, 1), IPAddress(4, 3, 2, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID, NULL, apChannel, false); //no password!!!
  #if defined(STARBASE_LOLIN_WIFI_FIX )
    WiFi.setTxPower(WIFI_POWER_8_5dBm );
  #endif
  if (!apActive) // start captive portal if AP active
  {
    mdl->setUIValueV("nwstatus", "AP %s / %s @ %s", apSSID, "NULL", WiFi.softAPIP().toString().c_str());

    //for captive portal
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    apActive = true;

    mdls->newConnection = true; // send all modules connect notification
  }
}

void SysModNetwork::stopAP() {
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  apActive = false;
  ppf("Access point disabled (handle).\n");
}

void SysModNetwork::handleAP() {
  byte stac = 0;
  wifi_sta_list_t stationList;
  esp_wifi_ap_get_sta_list(&stationList);
  stac = stationList.num;
  if (stac != stacO) {
    stacO = stac;
    if (WiFi.status() != WL_CONNECTED) {
      ppf("Connected AP clients: %d %d\n", stac, WiFi.status());
      if (stac)
        WiFi.disconnect();        // disable search so that AP can work
      else
        initConnection();         // restart search
    }
  }
  dnsServer.processNextRequest(); //for captiveportal
}