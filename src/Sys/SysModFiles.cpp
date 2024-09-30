/*
   @title     StarBase
   @file      SysModFiles.cpp
   @date      20240819
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModFiles.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModModel.h"
#include "SysModSystem.h"

// #include <FS.h>

SysModFiles::SysModFiles() :SysModule("Files") {
  if (!LittleFS.begin(true)) { //true: formatOnFail
    ppf(" An Error has occurred while mounting File system");
    ppf(" fail\n");
    success = false;
  }
};

void SysModFiles::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name, 2101);

  JsonObject tableVar = ui->initTable(parentVar, "fileTbl", nullptr, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Files");
      ui->setComment(var, "List of files");
      return true;
    case onAdd:
      rowNr = fileNames.size();
      web->getResponseObject()["onAdd"]["rowNr"] = rowNr; //also done in callVarFun??
      //add a row with all defaults
      //tbd: File upload does not call onAdd (bug?)
      return true;
    case onDelete:
      if (rowNr != UINT8_MAX && rowNr < fileNames.size()) {
        const char * fileName = fileNames[rowNr].s;
        // ppf("fileTbl onDelete %s[%d] = %s %s\n", Variable(var).id(), rowNr, Variable(var).valueString().c_str(), fileName);
        this->removeFiles(fileName, false);

        #ifdef STARBASE_USERMOD_LIVE
          if (strstr(fileName, ".sc") != nullptr) {
            char name[32]="del:/"; //del:/ is recognized by LiveM->loop20ms
            strcat(name, fileName);
            ppf("sc file removed %s\n", name);
            strcpy(web->lastFileUpdated, name);
          }
        #endif

        // print->printVar(var);
        // ppf("\n");
      }
      return true;
    default: return false;
  }});

  ui->initTextVector(tableVar, "flName", &fileNames, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Name");
      return true;
    default: return false;
  }});

  ui->initNumber(tableVar, "flSize", &fileSizes, 0, UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Size (B)");
      return true;
    default: return false;
  }});

  // ui->initURL(tableVar, "flLink", nullptr, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
  //   case onSetValue:
  //     for (forUnsigned8 rowNr = 0; rowNr < fileList.size(); rowNr++) {
  //       char urlString[32] = "file/";
  //       strncat(urlString, fileList[rowNr].name, sizeof(urlString)-1);
  //       mdl->setValue(var, JsonString(urlString, JsonString::Copied), rowNr);
  //     }
  //     return true;
  //   case onUI:
  //     ui->setLabel(var, "Show");
  //     return true;
  //   default: return false;
  // }});

  ui->initNumber(tableVar, "flTime", &fileTimes, 0, UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Time");
      return true;
    default: return false;
  }});

  //readonly = true, but button must be pressable (done in index.js)
  ui->initFileEditVector(tableVar, "flEdit", &fileNames, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Edit");
      return true;
    default: return false;
  }});

  ui->initFileUpload(parentVar, "upload", nullptr, UINT16_MAX, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Upload File");
    default: return false;
  }});

  ui->initProgress(parentVar, "drsize", files->usedBytes(), 0, files->totalBytes(), true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "FS Size");
      return true;
    case onChange:
      var["max"] = files->totalBytes(); //makes sense?
      web->addResponse(var, "comment", "%d / %d B", files->usedBytes(), files->totalBytes());
      return true;
    default: return false;
  }});

}

void SysModFiles::loop20ms() {

  if (filesChanged) {
    filesChanged = false;

    File root = LittleFS.open("/");
    File file = root.openNextFile();

    //repopulate file list
    fileNames.clear();
    fileSizes.clear();
    fileTimes.clear();
    uint8_t rowNr = 0;
    while (file) {

      while (rowNr >= fileNames.size()) fileNames.push_back(VectorString()); //create vector space if needed...
      strcpy(fileNames[rowNr].s, file.name());
      while (rowNr >= fileSizes.size()) fileSizes.push_back(UINT8_MAX); //create vector space if needed...
      fileSizes[rowNr] = file.size(); 
      while (rowNr >= fileTimes.size()) fileTimes.push_back(UINT8_MAX); //create vector space if needed...
      fileTimes[rowNr] = file.getLastWrite(); // - millis()/1000; if (details.time < 0) details.time = 0;

      file.close();
      file = root.openNextFile();
      rowNr++;
    }
    root.close();

    mdl->setValue("drsize", files->usedBytes());

    uint8_t rowNrL = 0;
    for (VectorString name: fileNames) {
      mdl->setValue("flName", JsonString(name.s, JsonString::Copied), rowNrL);
      mdl->setValue("flEdit", JsonString(name.s, JsonString::Copied), rowNrL);
      rowNrL++;
    }
    rowNrL = 0; for (uint16_t size: fileSizes) mdl->setValue("flSize", size, rowNrL++);
    rowNrL = 0; for (uint16_t time: fileTimes) mdl->setValue("flTime", time, rowNrL++);
  }
}

void SysModFiles::loop10s() {
  mdl->setValue("drsize", files->usedBytes());
}

bool SysModFiles::remove(const char * path) {
  ppf("File remove %s\n", path);
  return LittleFS.remove(path);
  filesChanged = true;
}

size_t SysModFiles::usedBytes() {
  return LittleFS.usedBytes();
}

size_t SysModFiles::totalBytes() {
  return LittleFS.totalBytes();
}

File SysModFiles::open(const char * path, const char * mode, const bool create) {
  return LittleFS.open(path, mode, create);
}

// https://techtutorialsx.com/2019/02/24/esp32-arduino-listing-files-in-a-spiffs-file-system-specific-path/
void SysModFiles::dirToJson(JsonArray array, bool nameOnly, const char * filter) {
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {

    if (filter == nullptr || strstr(file.name(), filter) != nullptr) {
      if (nameOnly) {
        array.add(JsonString(file.name(), JsonString::Copied));
      }
      else {
        JsonArray row = array.add<JsonArray>();
        row.add(JsonString(file.name(), JsonString::Copied));
        row.add(file.size());
        char urlString[32] = "file/";
        strncat(urlString, file.name(), sizeof(urlString)-1);
        row.add(JsonString(urlString, JsonString::Copied));
      }
      // ppf("FILE: %s %d\n", file.name(), file.size());
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}

bool SysModFiles::seqNrToName(char * fileName, size_t seqNr, const char * filter) {

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  size_t counter = 0;
  while (file) {
    if (filter == nullptr || strstr(file.name(), filter) != nullptr) {
      if (counter == seqNr) {
        // ppf("seqNrToName: %d %s %d\n", seqNr, file.name(), file.size());
        root.close();
        strncat(fileName, "/", 31); //add root prefix, fileName is 32 bytes but sizeof doesn't know so cheating
        strncat(fileName, file.name(), 31);
        file.close();
        return true;
      }
      counter++;
    }
    file.close();
    file = root.openNextFile();
  }

  root.close();
  return false;
}

bool SysModFiles::readObjectFromFile(const char* path, JsonDocument* dest) {
  // if (doCloseFile) closeFile();
  File f = open(path, "r");
  if (!f) {
    ppf("File %s open not successful\n", path);
    return false;
  }
  else { 
    ppf("File %s open to read, size %d bytes\n", path, (int)f.size());
    DeserializationError error = deserializeJson(*dest, f, DeserializationOption::NestingLimit(20)); //StarBase requires more then 10
    if (error) {
      print->printJDocInfo("readObjectFromFile", *dest);
      ppf("readObjectFromFile deserializeJson failed with code %s\n", error.c_str());
      f.close();
      return false;
    } else {
      f.close();
      return true;
    }
  }
}

//candidate for deletion as taken over by StarJson
// bool SysModFiles::writeObjectToFile(const char* path, JsonDocument* dest) {
//   File f = open(path, "w");
//   if (f) {
//     print->println("  success");
//     serializeJson(*dest, f);
//     f.close();
//     filesChanged = true;
//     return true;
//   } else {
//     print->println("  fail");
//     return false;
//   }
// }

void SysModFiles::removeFiles(const char * filter, bool reverse) {
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {
    if (filter == nullptr || reverse?strstr(file.name(), filter) == nullptr: strstr(file.name(), filter) != nullptr) {
      char fileName[32] = "/";
      strncat(fileName, file.name(), sizeof(fileName)-1);
      file.close(); //close otherwise not removeable
      remove(fileName);
    }
    else
      file.close();

    file = root.openNextFile();
  }

  root.close();
}