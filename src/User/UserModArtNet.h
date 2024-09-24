/*
   @title     StarLight
   @file      UserModArtNet.h
   @date      20240819
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#define ARTNET_DEFAULT_PORT 6454

const size_t ART_NET_HEADER_SIZE = 12;
const byte   ART_NET_HEADER[] PROGMEM = {0x41,0x72,0x74,0x2d,0x4e,0x65,0x74,0x00,0x00,0x50,0x00,0x0e};

class UserModArtNet:public SysModule {

public:

  IPAddress targetIp; //tbd: targetip also configurable from fixtures and artnet instead of pin output
  std::vector<uint16_t> hardware_outputs = {1024,1024,1024,1024,1024,1024,1024,1024};
  std::vector<uint16_t> hardware_outputs_universe_start = { 0,7,14,21,28,35,42,49 }; //7*170 = 1190 leds => last universe not completely used

  UserModArtNet() :SysModule("ArtNet") {
    isEnabled = false; //default off
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6100);

    ui->initNumber(parentVar, "artIP", 11, 0, 255, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        ui->setLabel(var, "Target IP");
        ui->setComment(var, "IP to send data to");
        return true; }
      case onChange: {
        uint8_t value = var["value"];
        targetIp[3] = value;
        return true; }
      default: return false;
    }});

    JsonObject tableVar = ui->initTable(parentVar, "anTbl", nullptr, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Outputs");
        return true;
      case onAdd:
        web->getResponseObject()["onAdd"]["rowNr"] = rowNr;
        return true;
      case onDelete:
        // web->getResponseObject()["onDelete"]["rowNr"] = rowNr;
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "anStart", &hardware_outputs_universe_start, 0, UINT16_MAX, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Start");
        ui->setComment(var, "Start universe");
        return true;
      default: return false;
    }});
    ui->initNumber(tableVar, "anSize", &hardware_outputs, 0, UINT16_MAX, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Size");
        ui->setComment(var, "# pixels");
        return true;
      default: return false;
    }});

  }

  void loop20ms() {
    // SysModule::loop();

    if(!mdls->isConnected) return;

    targetIp[0] = net->localIP()[0];
    targetIp[1] = net->localIP()[1];
    targetIp[2] = net->localIP()[2];

    if(!targetIp) return;

    if(!eff->newFrame) return;

    uint8_t bri = mdl->linearToLogarithm(fix->bri);

    // calculate the number of UDP packets we need to send

    byte packet_buffer[ART_NET_HEADER_SIZE + 6 + 512];
    memcpy(packet_buffer, ART_NET_HEADER, 12); // copy in the Art-Net header.

    AsyncUDP artnetudp;// AsyncUDP so we can just blast packets.

    const uint_fast16_t ARTNET_CHANNELS_PER_PACKET = 510; // 512/4=128 RGBW LEDs, 510/3=170 RGB LEDs

    uint_fast16_t bufferOffset = 0;
    uint_fast16_t hardware_output_universe = 0;
    
    sequenceNumber++;

    if (sequenceNumber == 0) sequenceNumber = 1; // just in case, as 0 is considered "Sequence not in use"
    if (sequenceNumber > 255) sequenceNumber = 1;
    
    for (uint_fast16_t hardware_output = 0; hardware_output < hardware_outputs.size(); hardware_output++) { //loop over all outputs
      
      if (bufferOffset > eff->fixture.nrOfLeds * sizeof(CRGB)) {
        // This stop is reached if we don't have enough pixels for the defined Art-Net output.
        return; // stop when we hit end of LEDs
      }

      hardware_output_universe = hardware_outputs_universe_start[hardware_output];

      uint_fast16_t channels_remaining = hardware_outputs[hardware_output] * sizeof(CRGB);

      while (channels_remaining > 0) {
        
        uint_fast16_t packetSize = ARTNET_CHANNELS_PER_PACKET;

        if (channels_remaining < ARTNET_CHANNELS_PER_PACKET) {
          packetSize = channels_remaining;
          channels_remaining = 0;
        } else {
          channels_remaining -= packetSize;
        }

        // set the parts of the Art-Net packet header that change:
        packet_buffer[12] = sequenceNumber;
        packet_buffer[14] = hardware_output_universe;
        packet_buffer[16] = packetSize >> 8;
        packet_buffer[17] = packetSize;

        // bulk copy the buffer range to the packet buffer after the header 
        memcpy(packet_buffer+18, (&eff->fixture.ledsP[0].r)+bufferOffset, packetSize); //start from the first byte of ledsP[0]

        for (int i = 18; i < packetSize+18; i+=sizeof(CRGB)) {
          // set brightness all at once - seems slightly faster than scale8()?
          // for some reason, doing 3/4 at a time is 200 micros faster than 1 at a time.
          packet_buffer[i] = (packet_buffer[i] * bri) >> 8;
          packet_buffer[i+1] = (packet_buffer[i+1] * bri) >> 8;
          packet_buffer[i+2] = (packet_buffer[i+2] * bri) >> 8; 
        }

        bufferOffset += packetSize;
        
        if (!artnetudp.writeTo(packet_buffer, packetSize+18, targetIp, ARTNET_DEFAULT_PORT)) {
          ppf("🐛");
          return; // borked
        }

        web->sendUDPCounter++;
        web->sendUDPBytes+=packetSize+18;

        hardware_output_universe++;
      }
    }
  } //loop

  private:
    size_t sequenceNumber = 0;

};

extern UserModArtNet *artnetmod;