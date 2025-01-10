/*
   @title     StarBase
   @file      main.cpp
   @date      20241219
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/


#ifdef STARBASE_MAIN
  #include "mainStar.h"
  void setup() {
    setupStar();
  }

  //loop all modules
  void loop() {
    loopStar();
  }
#endif