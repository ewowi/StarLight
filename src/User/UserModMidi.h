/*
   @title     StarBase
   @file      UserModMidi.h
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

// #include <usbMidiHost.ino>

class UserModMidi:public SysModule {

public:

  UserModMidi() :SysModule("Midi") {
  };

  void setup() {
    SysModule::setup();

    // UsbMidi_Setup();
    // esp32_usb_midi.begin();  // Initialize USB MIDI

  }

  void onOffChanged() {
    if (mdls->isConnected && isEnabled) {


    } else {
    }
  }

  void loop() {
    // if (esp32_usb_midi.available()) {
    //     esp32_usb_midi.read();  // Read MIDI data
    // }
  }

};

extern UserModMidi *midi;