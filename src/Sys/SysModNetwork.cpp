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

#ifdef STARBASE_ETHERNET
  #include <ETH.h>
#endif

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

  JsonObject currentVar;

  #ifdef STARBASE_ETHERNET

    currentVar = ui->initCheckBox(parentVar, "ethOn", false, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Ethernet");
      return true;
    case onLoop1s:
      //initEthernet not done in onChange as initEthernet needs a bit of a delay
      if (!ethActive && mdl->getValue(var).as<bool>())
        initEthernet();
    default: return false;
    }});

    //set olimex default as details hidden then
    ui->initSelect(currentVar, "ethConfig", 1, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI: {
        ui->setLabel(var, "Config");
        JsonArray options = ui->setOptions(var);
        options.add("Manual");
        options.add("Olimex ESP32 Gateway");
        return true;
      }
      case onChange:
        Variable(var).preDetails();
        mdl->setValueRowNr = rowNr;

        if (var["value"] == 0) {//manual
          ui->initNumber(var, "ethaddr", (uint16_t)0, 0, 255, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
            case onUI:
              ui->setLabel(var, "Address");
              return true;
            case onChange:
              ethActive = false;
              return true;
            default: return false;
          }});
          ui->initPin(var, "ethpower", 14, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
            case onUI:
              ui->setLabel(var, "Power");
              return true;
            case onChange:
              ethActive = false;
              return true;
            default: return false;
          }});
          ui->initPin(var, "ethmdc", 23, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
            case onUI:
              ui->setLabel(var, "mdc");
              return true;
            case onChange:
              ethActive = false;
              return true;
            default: return false;
          }});
          ui->initPin(var, "ethmdio", 18, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
            case onUI:
              ui->setLabel(var, "mdio");
              return true;
            case onChange:
              ethActive = false;
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
              ethActive = false;
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
              ethActive = false;
              return true;
            default: return false;
          }});
        }

        Variable(var).postDetails(rowNr);
        mdl->setValueRowNr = UINT8_MAX;

        ethActive = false;
        // initEthernet(); //try to connect

        return true;
      default: return false;
    }});

    ui->initText(currentVar, "etStatus", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Status");
        return true;
      case onLoop1s:
          mdl->setValue(var, "%s %s", ethActive?ETH.localIP()[0]?"🟢":"🟠":"🛑", ethActive?ETH.localIP().toString().c_str():"inactive");
        return true;
      default: return false;
    }});

  #endif

  currentVar = ui->initCheckBox(parentVar, "wifiOn", true, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "WiFi");
      return true;
    case onChange:
      if (var["value"].as<bool>())
        initWiFiConnection();
      else
        stopWiFiConnection();
      return true;
    default: return false;
  }});

  ui->initText(currentVar, "ssid", "", 31, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onChange:
      if (mdl->getValue("Network", "wifiOn").as<bool>()) {
        stopWiFiConnection();
        initWiFiConnection();
      }
      return true;
    default: return false;
  }});

  ui->initPassword(currentVar, "pw", "", 63, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Password");
      return true;
    case onChange:
      if (mdl->getValue("Network", "wifiOn").as<bool>()) {
        stopWiFiConnection();
        initWiFiConnection();
      }
      return true;
    default: return false;
  }});

  ui->initText(currentVar, "rssi", nullptr, 32, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Signal");
      return true;
    case onLoop1s:
      mdl->setValue(var, "%d dBm", WiFi.RSSI(), 0); //0 is to force format overload used
      return true;
    default: return false;
  }});

  ui->initText(currentVar, "wfStatus", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Status");
      return true;
    case onLoop1s:
      mdl->setValue(var, "%s %s (s:%d)",  wfActive?WiFi.localIP()[0]?"🟢":"🟠":"🛑", wfActive?WiFi.localIP().toString().c_str():"inactive", WiFi.status());
      return true;
    default: return false;
  }});

  currentVar = ui->initCheckBox(parentVar, "apOn", false, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "AP");
      return true;
    case onChange:
      if (var["value"].as<bool>())
        initAP();
      else
        stopAP();
      return true;
    // case onLoop1s:
      // if (!apActive && mdl->getValue(var).as<bool>()) {
      //   stopAP();
      //   initAP();
      // }
      // return true;
    default: return false;
  }});
  ui->initText(currentVar, "apStatus", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Status");
      return true;
    case onLoop1s:
      mdl->setValue(var, "%s %s",  apActive?WiFi.softAPIP()[0]?"🟢":"🟠":"🛑", apActive?WiFi.softAPIP().toString().c_str():"inactive");
      return true;
    default: return false;
  }});

}

void SysModNetwork::loop1s() {

  if (apActive)
    handleAP();
}

void SysModNetwork::loop10s() {
  if (millis() < 60000) {// show IP the first minute
    #ifdef STARBASE_ETHERNET
      ppf("e:%s ", ETH.localIP().toString().c_str());
    #endif
    ppf("w:%s a:%s\n", WiFi.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str()); // show IP the first minute
  }

  //initAP if no IP's, stopAP if IP's and apOn is false
  #ifdef STARBASE_ETHERNET
    //if no ip's found, init AP
    if (ETH.localIP()[0] == 0 && WiFi.localIP()[0] == 0) {
  #else
    if (WiFi.localIP()[0] == 0) {
  #endif
    if (!apActive && WiFi.softAPIP()[0] == 0) {
      ppf("no IP's found -> initAP\n");
      initAP();
    }
  } else {
    if (apActive && !mdl->getValue("Network", "apOn").as<bool>()) {
      ppf("IP's found -> stopAP (%s %s)\n", ETH.localIP().toString().c_str(), WiFi.localIP().toString().c_str());
      stopAP();
    }
  }
}

IPAddress SysModNetwork::localIP() {
  #ifdef STARBASE_ETHERNET
    if (ETH.localIP()[0] != 0)
      return ETH.localIP();
  #endif
  if (WiFi.localIP()[0] != 0)
    return WiFi.localIP();
  if (WiFi.softAPIP()[0] != 0)
    return WiFi.softAPIP();
    
  return IPAddress();
}

void SysModNetwork::initWiFiConnection() {

  if (wfActive ) //allready success && WiFi.localIP()[0] != 0
    return;

  WiFi.disconnect(true);        // close old connections

  //close ap if not ap
  // if (!apActive) {
  //   stopAP();
  //   WiFi.mode(WIFI_STA);
  // }

  const char * ssid = mdl->getValue("wifiOn", "ssid");
  const char * password = mdl->getValue("wifiOn", "pw");
  if (ssid && strlen(ssid)>0 && password) {
    char passXXX [64] = "";
    for (int i = 0; i < strlen(password); i++) strncat(passXXX, "*", sizeof(passXXX)-1);
    WiFi.begin(ssid, password);
    ppf("initWiFiConnection success %s / %s s:%d\n", ssid, passXXX, WiFi.status()); //6 is disconnected
    #if defined(STARBASE_LOLIN_WIFI_FIX )
      WiFi.setTxPower(WIFI_POWER_8_5dBm );
    #endif
    WiFi.setSleep(false);
    WiFi.setHostname(mdns->cmDNS); //use the mdns name (instance name or star-mac)

    wfActive = true;
    if (!connected) {
      connected = true;
      mdls->newConnection = true;
    }
    // }
    // else ppf("initWiFiConnection failed: status not connected\n");
  }
  else
    ppf("initWiFiConnection not succesful ssid:%s pw:%s s:%d\n", ssid?ssid:"No SSID", password?password:"No Password", WiFi.status());
}

void SysModNetwork::stopWiFiConnection() {
  if (!wfActive) return;

  WiFi.disconnect(true);
  wfActive = false;
  ppf("stopWiFiConnection s:%d\n", WiFi.status());
}

void SysModNetwork::initAP() {
  if (apActive) // && WiFi.softAPIP()[0] != 0
    return;

  const char * apSSID = mdl->getValue("System", "name");
  if (WiFi.softAPConfig(IPAddress(4, 3, 2, 1), IPAddress(4, 3, 2, 1), IPAddress(255, 255, 255, 0))
      && WiFi.softAP(apSSID, NULL, apChannel, false)) { //no password!!!
    ppf("AP success %s %s s:%d\n", apSSID, WiFi.softAPIP().toString().c_str(), WiFi.status()); //6 is disconnected
    #if defined(STARBASE_LOLIN_WIFI_FIX )
      WiFi.setTxPower(WIFI_POWER_8_5dBm );
    #endif
    //for captive portal
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    apActive = true;

    if (!connected) {
      connected = true;
      mdls->newConnection = true;
    }
  }
  else ppf("initAP not successful\n");
}

void SysModNetwork::stopAP() {
  if (!apActive) return; //allready stopped
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  apActive = false;
  ppf("stopAP s:%d\n", WiFi.status());
}

void SysModNetwork::handleAP() {
  byte stac = 0;
  wifi_sta_list_t stationList;
  esp_wifi_ap_get_sta_list(&stationList);
  stac = stationList.num;
  if (stac != stationCount) { //stationCount changed
    stationCount = stac;
    if (WiFi.status() != WL_CONNECTED) {
      ppf("handleAP changed: #:%d s:%d\n", stac, WiFi.status()); //6 is disconnected
      if (stac)
        WiFi.disconnect();        // disable search so that AP can work
      else
        initWiFiConnection();         // restart search
    }
  }
  dnsServer.processNextRequest(); //for captiveportal
}

#ifdef STARBASE_ETHERNET
typedef struct EthernetSettings {
  uint8_t        eth_address;
  int            eth_power;
  int            eth_mdc;
  int            eth_mdio;
  eth_phy_type_t eth_type;
  // #ifndef CONFIG_IDF_TARGET_ESP32S3
  eth_clock_mode_t eth_clk_mode;
  // #endif
} ethernet_settings;

bool SysModNetwork::initEthernet() {

  if (ethActive) { //  && ETH.localIP()[0] != 0
    return false;
  }

  pinsM->deallocatePin(UINT8_MAX, "Eth");

  uint8_t ethernet = mdl->getValue("ethOn", "ethConfig");

  ethernet_settings es;

  bool result;

  switch (ethernet) {
    case 0: //manual
    {
      es = {
        mdl->getValue("ethOn", "ethaddr"),			              // eth_address,
        mdl->getValue("ethOn", "ethpower"),			              // eth_power,
        mdl->getValue("ethOn", "ethmdc"),			              // eth_mdc,
        mdl->getValue("ethOn", "ethmdio"),			              // eth_mdio,
        mdl->getValue("ethOn", "ethtype"),      // eth_type,
        mdl->getValue("ethOn", "ethclkmode")	// eth_clk_mode
      };

    }
    break;
    case 1: //Olimex
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

  if (ETH.begin(
                (uint8_t) es.eth_address,
                (int)     es.eth_power,
                (int)     es.eth_mdc,
                (int)     es.eth_mdio,
                (eth_phy_type_t)   es.eth_type,
                (eth_clock_mode_t) es.eth_clk_mode
                )) {
                  
    ETH.setHostname(mdns->cmDNS);

    ethActive = true;

    //tbd: make eth vars readonly as once connected cannot be changed anymore
    // mdl->findVar("ethOn")["ro"] = true; //not possible to change anymore if connected

    if (!connected) {
      connected = true;
      mdls->newConnection = true;
    }

    ppf("initEthernet success %s s:%d\n", ETH.localIP().toString().c_str(), WiFi.status());  // WLEDMM
    return true;
  }
  else {
    ppf("initEthernet not successful s:%d\n", WiFi.status());
    // de-allocate the allocated pins
    // for (managed_pin_type mpt : pinsToAllocate) {
    //   pinManager.deallocatePin(mpt.pin, PinOwner::Ethernet);
    // }
    return false;
  }
}

void SysModNetwork::stopEthernet() {
  if (!ethActive) return;
  //not possible to stop ...
  // esp_eth_stop(ETH.eth_handle);
  // ethActive = false;
}

#endif