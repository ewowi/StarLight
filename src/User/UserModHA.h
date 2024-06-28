/*
   @title     StarBase
   @file      UserModHA.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include <ArduinoHA.h>

// Basic example of MQTT connectivity to Home Assistant.
// Add HALight, HASelect etc as required
class UserModHA:public SysModule {

public:

  UserModHA() :SysModule("Home Assistant support") {
    isEnabled = false;
  };

  void setup() override {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6300);

    ui->initText(parentVar, "mqttAddr");
    ui->initText(parentVar, "mqttUser");
    ui->initText(parentVar, "mqttPass");
  }


  void connectedChanged() {
    ppf("connectedChanged\n");
    if (mdls->isConnected) {
      // set device's details (optional)
      device.setName(mdl->getValue("instance"));
      device.setSoftwareVersion(_INIT(TOSTRING(VERSION)));
   }

    byte mac[6];
    WiFi.macAddress(mac);
    device.setUniqueId(mac, sizeof(mac));

    String mqttAddr = mdl->getValue("mqttAddr");
    String mqttUser = mdl->getValue("mqttUser");
    if(mqttUser == "null" || mqttUser == nullptr) mqttUser = "";
    String mqttPass = mdl->getValue("mqttPass");
    if(mqttPass == "null" || mqttPass == nullptr) mqttPass = "";

    IPAddress ip;
    if(ip.fromString(mqttAddr)) {
      if(mqttUser == "") {
        ppf("mqtt->begin('%s')\n", mqttAddr.c_str());
        mqtt->begin(ip);
      }
      else {
        ppf("WARNING - untested mqtt->begin('%s', '%s', pass)\n", mqttAddr.c_str(), mqttUser.c_str());
        mqtt->begin(ip, mqttUser.c_str(), mqttPass.c_str());  
      }
      started = true;
    }
    else {
      ppf("Failed to parse %s to IP\n", mqttAddr.c_str());
    }

  }

  void loop20ms() override {
    mqtt->loop();
  }

  void loop10s() override {
    if(!started) return;
    testSensor->setValue((uint32_t) (millis() / 1000));
  }

  private:
    WiFiClient client;
    HADevice device;
    HAMqtt* mqtt = new HAMqtt(client, device);
    HASensorNumber* testSensor = new HASensorNumber("uptime");
    bool started = false;
};

extern UserModHA *hamod;