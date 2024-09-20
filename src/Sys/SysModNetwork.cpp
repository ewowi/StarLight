/*
   @title     StarBase
   @file      SysModNetwork.cpp
   @date      20240819
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
#include "SysModPins.h"

#include <ETH.h>


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

  ui->initText(parentVar, "rssi", nullptr, 32, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Wifi signal");
      return true;
    default: return false;
  }});

  ui->initSelect(parentVar, "ethernet", 0, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case onUI: {
      JsonArray options = ui->setOptions(var);
      options.add("None");
      options.add("Manual");
      options.add("Olimex ESP32 Gateway");
      options.add("Other");
      options.add("Another");
      options.add("tbd");
      return true;
    }
    case onChange:
      Variable(var).preDetails();
      mdl->setValueRowNr = rowNr;

      if (var["value"] == 1) {//manual
        ui->initNumber(var, "ethaddr", (uint16_t)0, 0, 255, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
          case onUI:
            ui->setLabel(var, "Address");
            return true;
          case onChange:
            successfullyConfiguredEthernet = false;
            return true;
          default: return false;
        }});
        ui->initPin(var, "ethpower", 14, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
          case onUI:
            ui->setLabel(var, "Power");
            return true;
          case onChange:
            successfullyConfiguredEthernet = false;
            return true;
          default: return false;
        }});
        ui->initPin(var, "ethmdc", 23, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
          case onUI:
            ui->setLabel(var, "mdc");
            return true;
          case onChange:
            successfullyConfiguredEthernet = false;
            return true;
          default: return false;
        }});
        ui->initPin(var, "ethmdio", 18, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
          case onUI:
            ui->setLabel(var, "mdio");
            return true;
          case onChange:
            successfullyConfiguredEthernet = false;
            return true;
          default: return false;
        }});
        ui->initSelect(var, "ethtype", ETH_PHY_LAN8720, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
          case onUI: {
            ui->setLabel(var, "Type");
            JsonArray options = ui->setOptions(var);
            options.add("LAN8720");
            options.add("TLK110");
            options.add("RTL8201");
            options.add("DP83848");
            options.add("DM9051");
            options.add("KSZ8041");
            options.add("KSZ8081");
            options.add("MAX");
            return true;
          }
          case onChange:
            successfullyConfiguredEthernet = false;
            return true;
          default: return false;
        }});
        ui->initSelect(var, "ethclkmode", ETH_CLOCK_GPIO17_OUT, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
          case onUI: {
            ui->setLabel(var, "Clock mode");
            JsonArray options = ui->setOptions(var);
            options.add("GPIO0_IN");
            options.add("GPIO0_OUT");
            options.add("GPIO16_OUT");
            options.add("GPIO17_OUT");
            return true;
          }
          case onChange:
            successfullyConfiguredEthernet = false;
            return true;
          default: return false;
        }});
      }

      Variable(var).postDetails(rowNr);
      mdl->setValueRowNr = UINT8_MAX;

      successfullyConfiguredEthernet = false;
      // initEthernet(); //try to connect

      return true;
    default: return false;
  }});

  ui->initText(parentVar, "nwstatus", nullptr, 32, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Status");
      return true;
    default: return false;
  }});

}

void SysModNetwork::loop1s() {
  handleConnection(); //once per second is enough
  mdl->setUIValueV("rssi", "%d dBm", WiFi.RSSI());
  mdl->setUIValueV("nwstatus", "Connected %s (e:%s s:%s)", WiFi.localIP().toString().c_str(), ETH.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str());
  initEthernet();
}

void SysModNetwork::loop10s() {
  if (millis() < 60000)
    ppf("%s %s %s\n", WiFi.localIP().toString().c_str(), ETH.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str()); // show IP the first minute
}

void SysModNetwork::handleConnection() {
  if (lastReconnectAttempt == 0) { // do this only once
    ppf("lastReconnectAttempt == 0\n");
    initConnection(); //WiFi connection
    return;
  }

  if (apActive) {
    handleAP();
  }

  //if not connected // || ETH.localIP()[0] != 0
  if (!((WiFi.localIP()[0] != 0 && WiFi.status() == WL_CONNECTED))) { //!Network.isConfirmedConnection()
    if (isConfirmedConnection) { //should not be confirmed as not connected -> lost connection -> retry
      ppf("Disconnected!\n");
      initConnection(); //WiFi connection
    }

    //if no connection for more then 6 seconds (was 12)
    if (!apActive && millis() - lastReconnectAttempt > 6000 ) { //&& (!wasConnected || apBehavior == AP_BEHAVIOR_NO_CONN)
      ppf("Not connected AP.\n");
      initAP();
    }
  } else if (!isConfirmedConnection) { //newly connected
    mdl->setUIValueV("nwstatus", "Connected %s (e:%s s:%s)", WiFi.localIP().toString().c_str(), ETH.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str());
    ppf("Connected %s %s %s\n", WiFi.localIP().toString().c_str(), ETH.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str());

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
    ppf("initConnection error ssid:%s pw:%s\n", ssid?ssid:"No SSID", password?password:"No Password");

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
      ppf("Connected AP clients: %d %d\n", stac, WiFi.status()); //6 is disconnected
      if (stac)
        WiFi.disconnect();        // disable search so that AP can work
      else
        initConnection();         // restart search
    }
  }
  dnsServer.processNextRequest(); //for captiveportal
}

typedef struct EthernetSettings {
  uint8_t        eth_address;
  int            eth_power;
  int            eth_mdc;
  int            eth_mdio;
  eth_phy_type_t eth_type;
  #ifndef CONFIG_IDF_TARGET_ESP32S3
  eth_clock_mode_t eth_clk_mode;
  #endif
} ethernet_settings;

bool SysModNetwork::initEthernet() {

  if (successfullyConfiguredEthernet) {
    // DEBUG_PRINTLN(F("initE: ETH already successfully configured, ignoring"));
    return false;
  }

  pinsM->deallocatePin(UINT8_MAX, "Eth");

  uint8_t ethernet = mdl->getValue("ethernet");

  ethernet_settings es;

  bool result;

  switch (ethernet) {
    case 0: //none
      return false;
    case 1: //manual
    {
      es = {
        mdl->getValue("ethaddr"),			              // eth_address,
        mdl->getValue("ethpower"),			              // eth_power,
        mdl->getValue("ethmdc"),			              // eth_mdc,
        mdl->getValue("ethmdio"),			              // eth_mdio,
        mdl->getValue("ethtype"),      // eth_type,
        mdl->getValue("ethclkmode")	// eth_clk_mode
      };

    }
    break;
    case 2: //Olimex
    {
      //WLEDMM: Olimex-ESP32-Gateway (like QuinLed-ESP32-Ethernet
      es = {
        0,			              // eth_address,
        5,			              // eth_power,
        23,			              // eth_mdc,
        18,			              // eth_mdio,
        ETH_PHY_LAN8720,      // eth_type,
        ETH_CLOCK_GPIO17_OUT	// eth_clk_mode
      };
    }
    break;
    default:
      es = {
        0,			              // eth_address,
        5,			              // eth_power,
        23,			              // eth_mdc,
        18,			              // eth_mdio,
        ETH_PHY_LAN8720,      // eth_type,
        ETH_CLOCK_GPIO17_OUT	// eth_clk_mode
      };
  }

  // looks like not needed until now (at least not for Olimex)
  // if(es.eth_power>0 && es.eth_type==ETH_PHY_LAN8720) {
  //   pinMode(es.eth_power, OUTPUT);
  //   digitalWrite(es.eth_power, 0);
  //   delayMicroseconds(150);
  //   digitalWrite(es.eth_power, 1);
  //   delayMicroseconds(10);
  // }

  pinsM->allocatePin(es.eth_power, "Eth", "power");
  pinsM->allocatePin(es.eth_mdc, "Eth", "mdc");
  pinsM->allocatePin(es.eth_mdio, "Eth", "mdio");

  // update the clock pin....
  if (es.eth_clk_mode == ETH_CLOCK_GPIO0_IN) {
    pinsM->allocatePin(0, "Eth", "clock");
  } else if (es.eth_clk_mode == ETH_CLOCK_GPIO0_OUT) {
    pinsM->allocatePin(0, "Eth", "clock");
  } else if (es.eth_clk_mode == ETH_CLOCK_GPIO16_OUT) {
    pinsM->allocatePin(16, "Eth", "clock");
  } else if (es.eth_clk_mode == ETH_CLOCK_GPIO17_OUT) {
    pinsM->allocatePin(17, "Eth", "clock");
  } else {
    ppf("dev initE: Failing due to invalid eth_clk_mode (%d)\n", es.eth_clk_mode);
    return false;
  }

  if (!ETH.begin(
                (uint8_t) es.eth_address,
                (int)     es.eth_power,
                (int)     es.eth_mdc,
                (int)     es.eth_mdio,
                (eth_phy_type_t)   es.eth_type,
                (eth_clock_mode_t) es.eth_clk_mode
                )) {
    ppf("initC: ETH.begin() failed\n");
    // de-allocate the allocated pins
    // for (managed_pin_type mpt : pinsToAllocate) {
    //   pinManager.deallocatePin(mpt.pin, PinOwner::Ethernet);
    // }
    return false;
  }

  successfullyConfiguredEthernet = true;
  ppf("initC: *** Ethernet successfully configured! %s ***\n", ETH.localIP().toString().c_str());  // WLEDMM
  return true;

}