/*
   @title     StarLight
   @file      StarLightProjectionSets.h
   @date      20241014
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

class StarLightProjectionSets:public SysModule {

public:


  StarLightProjectionSets() :SysModule("ProjectionSets") {
    isEnabled = true;
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6500);

    JsonObject tableVar = ui->initTable(parentVar, "projectionSets");

    ui->initText(tableVar, "name", nullptr, 32);

    ui->initSelect(tableVar, "projection", 1, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      // case onSetValue:
      //   for (size_t rowNr = 0; rowNr < fixture.layers.size(); rowNr++)
      //     mdl->setValue(var, fixture.layers[rowNr]->projectionNr, rowNr);
      //   return true;
      case onUI: {
        ui->setComment(var, "How to project effect");

        JsonArray options = ui->setOptions(var);
        for (Projection *projection:eff->projections) {
          char buf[32] = "";
          strlcat(buf, projection->name(), sizeof(buf));
          // strlcat(buf, projection->dim()==_1D?" ┊":projection->dim()==_2D?" ▦":" 🧊");
          strlcat(buf, " ", sizeof(buf));
          strlcat(buf, projection->tags(), sizeof(buf));
          options.add(JsonString(buf, JsonString::Copied)); //copy!
        }
        return true; }
      case onChange: {

        //rowNr of projection sets table
        if (rowNr == UINT8_MAX) rowNr = 0; // in case effect without a rowNr

        uint8_t proValue = mdl->getValue(var, rowNr);

        if (proValue < eff->projections.size()) {

          Projection * projection = eff->projections[proValue];

          LedsLayer *leds = eff->fixture.layers[0]; //take the first for now ...

          Variable(var).preDetails(); //set all positive var N orders to negative
          mdl->setValueRowNr = rowNr;
          projection->controls(*leds, var); //not if None projection
          Variable(var).postDetails(rowNr);
          mdl->setValueRowNr = UINT8_MAX;
        }
        return true; }
      default: return false;
    }});

    // ui->initNumber(tableVar, "start", &hardware_outputs_universe_start, 0, UINT16_MAX, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    //   case onUI:
    //     ui->setComment(var, "Start universe");
    //     return true;
    //   default: return false;
    // }});
    // ui->initNumber(tableVar, "size", &hardware_outputs, 0, UINT16_MAX, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    //   case onUI:
    //     ui->setComment(var, "# pixels");
    //     return true;
    //   default: return false;
    // }});

  }


};

extern StarLightProjectionSets *proSets;