/*
   @title     StarBase
   @file      SysModules.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"

class SysModules {
public:
  bool newConnection = false;
  bool isConnected = false;

  SysModules();

  void setup();

  void loop();

  void reboot();

  void add(SysModule* module);

  void connectedChanged();

private:
  std::vector<SysModule *> modules;
  // unsigned long oneSecondMillis = 0;
  // unsigned long tenSecondMillis = millis() - 4500;
};

extern SysModules *mdls;