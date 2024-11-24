/*
   @title     StarBase
   @file      SysModNetwork.cpp
   @date      20241105
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

  const Variable parentVar = ui->initSysMod(Variable(), name, 3502);
  parentVar.var["s"] = true; //setup

  // Variable tableVar = ui->initTable(parentVar, "wfTbl", nullptr, false, [this](EventArguments) { //varEvent ro false: create and delete row possible
  //   variable.setComment("List of defined and available Wifi APs");
  // });

  Variable currentVar = ui->initCheckBox(parentVar, "wiFi", (bool3State)true, false, [this](EventArguments) { switch (eventType) {
    case onChange:
      if (variable.value().as<bool>())
        initWiFiConnection();
      else
        stopWiFiConnection();
      return true;
    default: return false;
  }});

  ui->initText(currentVar, "ssid", "", 31, false, [this](EventArguments) { switch (eventType) {
    case onChange:
      if (mdl->getValue("Network", "wiFi").as<bool>()) {
        stopWiFiConnection();
        initWiFiConnection();
      }
      return true;
    default: return false;
  }});

  ui->initPassword(currentVar, "password", "", 63, false, [this](EventArguments) { switch (eventType) {
    case onChange:
      if (mdl->getValue("Network", "wiFi").as<bool>()) {
        stopWiFiConnection();
        initWiFiConnection();
      }
      return true;
    default: return false;
  }});

  ui->initText(currentVar, "rssi", nullptr, 32, true, [](EventArguments) { switch (eventType) {
    case onLoop1s:
      variable.setValueF("%d dBm", WiFi.RSSI(), 0); //0 is to force format overload used
      return true;
    default: return false;
  }});

  ui->initText(currentVar, "status", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
    case onLoop1s:
      variable.setValueF("%s %s (s:%d)",  wfActive?WiFi.localIP()[0]?"🟢":"🟠":"🛑", wfActive?WiFi.localIP().toString().c_str():"inactive", WiFi.status());
      return true;
    default: return false;
  }});

  #ifdef STARBASE_ETHERNET

    currentVar = ui->initCheckBox(parentVar, "ethernet", (bool3State)false, false, [this](EventArguments) { switch (eventType) {
    case onLoop1s:
      //initEthernet not done in onChange as initEthernet needs a bit of a delay
      if (!ethActive && variable.getValue().as<bool>())
        initEthernet();
    default: return false;
    }});

    //set olimex default as details hidden then
    ui->initSelect(currentVar, "config", 1, false, [this](EventArguments) { switch (eventType) {
      case onUI: {
        JsonArray options = variable.setOptions();
        options.add("Manual");
        options.add("Olimex ESP32 Gateway");
        return true;
      }
      case onChange:
        variable.preDetails();
        mdl->setValueRowNr = rowNr;

        if (variable.value() == 0) {//manual
          ui->initNumber(variable, "address", (uint16_t)0, 0, 255, false, [this](EventArguments) { switch (eventType) {
            case onChange:
              ethActive = false;
              return true;
            default: return false;
          }});
          ui->initPin(variable, "power", 14, false, [this](EventArguments) { switch (eventType) {
            case onChange:
              ethActive = false;
              return true;
            default: return false;
          }});
          ui->initPin(variable, "mdc", 23, false, [this](EventArguments) { switch (eventType) {
            case onChange:
              ethActive = false;
              return true;
            default: return false;
          }});
          ui->initPin(variable, "mdio", 18, false, [this](EventArguments) { switch (eventType) {
            case onChange:
              ethActive = false;
              return true;
            default: return false;
          }});
          ui->initSelect(variable, "type", ETH_PHY_LAN8720, false, [this](EventArguments) { switch (eventType) {
            case onUI: {
              JsonArray options = variable.setOptions();
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
          ui->initSelect(variable, "clockMode", ETH_CLOCK_GPIO17_OUT, false, [this](EventArguments) { switch (eventType) {
            case onUI: {
              JsonArray options = variable.setOptions();
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

        variable.postDetails(rowNr);
        mdl->setValueRowNr = UINT8_MAX;

        ethActive = false;
        // initEthernet(); //try to connect

        return true;
      default: return false;
    }});

    ui->initText(currentVar, "status", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
      case onLoop1s:
          variable.setValueF("%s %s", ethActive?ETH.localIP()[0]?"🟢":"🟠":"🛑", ethActive?ETH.localIP().toString().c_str():"inactive");
        return true;
      default: return false;
    }});

  #endif

  currentVar = ui->initCheckBox(parentVar, "AP", (bool3State)false, false, [this](EventArguments) { switch (eventType) {
    case onChange:
      if (variable.value().as<bool>())
        initAP();
      else
        stopAP();
      return true;
    // case onLoop1s:
      // if (!apActive && variable.getValue().as<bool>()) {
      //   stopAP();
      //   initAP();
      // }
      // return true;
    default: return false;
  }});
  ui->initText(currentVar, "status", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
    case onLoop1s:
      variable.setValueF("%s %s",  apActive?WiFi.softAPIP()[0]?"🟢":"🟠":"🛑", apActive?WiFi.softAPIP().toString().c_str():"inactive");
      return true;
    default: return false;
  }});

}

void SysModNetwork::loop1s() {

  //string tests
  // char test1[] = "Hello";
  // ppf("test1=%s s:%d l:%d\n", test1, sizeof(test1), strlen(test1));

  // const char * test2 = mdl->getValue("wiFi", "ssid");
  // ppf("test2=%s  s:%d l:%d\n", test2, sizeof(test2), strlen(test2)); //test2 may not be nullptr, size is size of ptr

  // char test3[32] = "";
  // ppf("test3=%s  s:%d l:%d\n", test3, sizeof(test3), strlen(test3));

  // // const char *test4;
  // // ppf("test4=null l:%d\n", sizeof(test4), strlen(test4)); //this crashes


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
    if (apActive && !mdl->getValue("Network", "AP").as<bool>()) {
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

  const char * ssid = mdl->getValue("wiFi", "ssid");
  const char * password = mdl->getValue("wiFi", "password");
  if (ssid && strnlen(ssid, 128) > 0 && password && strnlen(password, 64) > 0) {
    char passXXX [64] = "";
    for (int i = 0; i < strnlen(password, 128); i++) strlcat(passXXX, "*", sizeof(passXXX));
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
    ppf("initWiFiConnection not successful ssid:%s pw:%s s:%d\n", ssid?ssid:"No SSID", password?password:"No Password", WiFi.status());
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
      && WiFi.softAP(apSSID, nullptr, apChannel, false)) { //no password!!!
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

  uint8_t ethernetConfig = mdl->getValue("ethernet", "config");

  ethernet_settings es;

  bool result;

  switch (ethernetConfig) {
    case 0: //manual
    {
      es = {
        mdl->getValue("ethernet", "address"),
        mdl->getValue("ethernet", "power"),
        mdl->getValue("ethernet", "mdc"),
        mdl->getValue("ethernet", "mdio"),
        mdl->getValue("ethernet", "type"),
        mdl->getValue("ethernet", "clockMode")
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
    // mdl->findVar("ethernet")["ro"] = true; //not possible to change anymore if connected

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