/*
   @title     StarMod
   @file      UserModHA.h
   @date      20240411
   @repo      https://github.com/ewowi/StarMod, submit changes to this file as PRs to ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include <ArduinoHA.h>

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

  static void onStateCommand(bool state, HALight* sender) {
      ppf("State: %s\n", state?"true":"false");

      mdl->setValue("on", state);

      sender->setState(state); // report state back to the Home Assistant
  }

  static void onBrightnessCommand(unsigned8 brightness, HALight* sender) {
      ppf("Brightness: %d\n", brightness);

      mdl->setValue("bri", brightness);

      sender->setBrightness(brightness); // report brightness back to the Home Assistant
  }

  static void onRGBColorCommand(HALight::RGBColor color, HALight* sender) {
      ppf("Red: %d Green: %d blue: %d\n", color.red, color.green, color.blue);

      mdl->setValue("Red", color.red);
      mdl->setValue("Green", color.green);
      mdl->setValue("Blue", color.blue);

      mdl->setValue("fx", 0); // Solid

      sender->setRGBColor(color); // report color back to the Home Assistant
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

    // configure light (optional)
    light->setName("LEDs");

    String options = "Solid";
    // for(int i = 0; i < eff->effects.effects.size(); i++) {
    //   options += ";";
    //   options += eff->effects.effects.at(i)->name();
    // }
    fxSelect->setOptions(options.c_str());

    ppf("Options: %s\n", options.c_str());

    // Optionally you can set retain flag for the HA commands
    // light.setRetain(true);

    // Maximum brightness level can be changed as follows:
    // light.setBrightnessScale(50);

    // Optionally you can enable optimistic mode for the HALight.
    // In this mode you won't need to report state back to the HA when commands are executed.
    // light.setOptimistic(true);

    // handle light states
    light->onStateCommand(onStateCommand);
    light->onBrightnessCommand(onBrightnessCommand);
    light->onRGBColorCommand(onRGBColorCommand);

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

  void loop() override {
    // SysModule::loop();
    mqtt->loop();
    light->setCurrentBrightness(mdl->getValue("bri"));
    light->setCurrentState(mdl->getValue("on"));
    int8_t fx = mdl->getValue("fx");
    fxSelect->setState(fx + 1);
  }

  void loop10s() override {
    if(!started) return;
    testSensor->setValue((uint32_t) (millis() / 1000));
  }

  private:
    WiFiClient client;
    HADevice device;
    HAMqtt* mqtt = new HAMqtt(client, device);
    HALight* light = new HALight(_INIT(TOSTRING(APP)), HALight::BrightnessFeature | HALight::RGBFeature);
    HASelect* fxSelect = new HASelect("fx");
    HASensorNumber* testSensor = new HASensorNumber("uptime");
    bool started = false;
};

extern UserModHA *hamod;