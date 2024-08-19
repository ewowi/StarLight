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

  UserModArtNet() :SysModule("ArtNet") {
    isEnabled = false; //default off
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6100);

    ui->initIP(parentVar, "artInst", UINT16_MAX, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        ui->setLabel(var, "Instance");
        ui->setComment(var, "Instance to send data");
        JsonArray options = ui->setOptions(var);
        //keyValueOption: add key (ip[3] and value instance name/ip)
        JsonArray keyValueOption = options.add<JsonArray>();
        keyValueOption.add(0);
        keyValueOption.add("no sync");
        for (InstanceInfo &instance : instances->instances) {
          if (instance.ip != WiFi.localIP()) {
            char option[64] = { 0 };
            strncpy(option, instance.name, sizeof(option)-1);
            strncat(option, " ", sizeof(option)-1);
            strncat(option, instance.ip.toString().c_str(), sizeof(option)-1);
            keyValueOption = options.add<JsonArray>();
            keyValueOption.add(instance.ip[3]);
            keyValueOption.add(option);
          }
        }
        return true; }
      case onChange: {
        uint8_t value = var["value"]; //ip[3] chosen
        for (InstanceInfo &instance : instances->instances) {
          if (instance.ip[3] == value) {
            targetIp = instance.ip;
            ppf("Start ArtNet to %s\n", targetIp.toString().c_str());
          }
        }
        return true; }
      default: return false;
    }}); //artInst
  }

  void loop() {
    // SysModule::loop();

    if(!mdls->isConnected) return;

    if(!targetIp) return;

    if(!eff->newFrame) return;

    // calculate the number of UDP packets we need to send
    bool isRGBW = false;

    const size_t channelCount = eff->fixture.nrOfLeds * (isRGBW?4:3); // 1 channel for every R,G,B,(W?) value
    const size_t ARTNET_CHANNELS_PER_PACKET = isRGBW?512:510; // 512/4=128 RGBW LEDs, 510/3=170 RGB LEDs
    const size_t packetCount = ((channelCount-1)/ARTNET_CHANNELS_PER_PACKET)+1;

    stackUnsigned32 channel = 0; 
    size_t bufferOffset = 0;

    sequenceNumber++;

    WiFiUDP ddpUdp;

    for (size_t currentPacket = 0; currentPacket < packetCount; currentPacket++) {

      if (sequenceNumber > 255) sequenceNumber = 0;

      if (!ddpUdp.beginPacket(targetIp, ARTNET_DEFAULT_PORT)) {
        ppf("Art-Net WiFiUDP.beginPacket returned an error\n");
        return; // borked
      }

      size_t packetSize = ARTNET_CHANNELS_PER_PACKET;

      if (currentPacket == (packetCount - 1U)) {
        // last packet
        if (channelCount % ARTNET_CHANNELS_PER_PACKET) {
          packetSize = channelCount % ARTNET_CHANNELS_PER_PACKET;
        }
      }

      byte header_buffer[ART_NET_HEADER_SIZE];
      memcpy_P(header_buffer, ART_NET_HEADER, ART_NET_HEADER_SIZE);
      ddpUdp.write(header_buffer, ART_NET_HEADER_SIZE); // This doesn't change. Hard coded ID, OpCode, and protocol version.
      ddpUdp.write(sequenceNumber & 0xFF); // sequence number. 1..255
      ddpUdp.write(0x00); // physical - more an FYI, not really used for anything. 0..3
      ddpUdp.write((currentPacket) & 0xFF); // Universe LSB. 1 full packet == 1 full universe, so just use current packet number.
      ddpUdp.write(0x00); // Universe MSB, unused.
      ddpUdp.write(0xFF & (packetSize >> 8)); // 16-bit length of channel data, MSB
      ddpUdp.write(0xFF & (packetSize     )); // 16-bit length of channel data, LSB

      for (size_t i = 0; i < eff->fixture.nrOfLeds; i++) {
        CRGB pixel = eff->fixture.ledsP[i];
        ddpUdp.write(scale8(pixel.r, fix->bri)); // R
        ddpUdp.write(scale8(pixel.g, fix->bri)); // G
        ddpUdp.write(scale8(pixel.b, fix->bri)); // B
        // if (isRGBW) ddpUdp.write(scale8(buffer[bufferOffset++], fix->bri)); // W
      }

      if (!ddpUdp.endPacket()) {
        ppf("Art-Net WiFiUDP.endPacket returned an error\n");
        return; // borked
      }
      channel += packetSize;
    }

  }

  private:
    size_t sequenceNumber = 0;

};

extern UserModArtNet *artnetmod;