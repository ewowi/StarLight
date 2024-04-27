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

#define BROKER_ADDR     IPAddress(192,168,178,42) //ewowi: could we scan that instead of hard coded?

class UserModHA:public SysModule {

public:

  UserModHA() :SysModule("Home Assistant support") {
    isEnabled = false;
  };

  void onStateCommand(bool state, HALight* sender) {
      ppf("State: %s\n", state?"true":"false");

      sender->setState(state); // report state back to the Home Assistant
  }

  void onBrightnessCommand(unsigned8 brightness, HALight* sender) {
      ppf("Brightness: %s\n", brightness);

      sender->setBrightness(brightness); // report brightness back to the Home Assistant
  }

  void onRGBColorCommand(HALight::RGBColor color, HALight* sender) {
      ppf("Red: %d Green: %d blue: %d\n", color.red, color.green, color.blue);

      sender->setRGBColor(color); // report color back to the Home Assistant
  }

  void connectedChanged() {
    if (mdls->isConnected) {
      // set device's details (optional)
      device.setName(_INIT(TOSTRING(APP)));
      device.setSoftwareVersion(_INIT(TOSTRING(VERSION)));
    }

    // configure light (optional)
    light->setName("LEDs");

    // Optionally you can set retain flag for the HA commands
    // light.setRetain(true);

    // Maximum brightness level can be changed as follows:
    // light.setBrightnessScale(50);

    // Optionally you can enable optimistic mode for the HALight.
    // In this mode you won't need to report state back to the HA when commands are executed.
    // light.setOptimistic(true);

    // handle light states
    light->onStateCommand(onStateCommand);
    light->onBrightnessCommand(onBrightnessCommand); // optional
    light->onRGBColorCommand(onRGBColorCommand); // optional

    mqtt->begin(BROKER_ADDR);
  }

  void loop() {
    // SysModule::loop();
    mqtt->loop();
  }

  private:
    WiFiClient client;
    HADevice device;
    HAMqtt* mqtt = new HAMqtt(client, device);
    HALight* light = new HALight(_INIT(TOSTRING(APP)), HALight::BrightnessFeature | HALight::RGBFeature);
};

extern UserModHA *hamod;