/*
   @title     StarBase
   @file      UserModMPU6050.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include "../Sys/SysModUI.h" //why needed here and not in other sysmods?
#include "../Sys/SysModPins.h"

#include <MPU6050_6Axis_MotionApps20.h>

//see https://github.com/ElectronicCats/mpu6050/blob/0281cd4532b36922f4d68a4cae70eca7aebe9988/examples/MPU6050_DMP6/MPU6050_DMP6.ino

class UserModMPU6050: public SysModule {

public:

  bool motionTrackingReady = false;  // set true if DMP init was successful

  Coord3D gyro; // in degrees (not radians)
  Coord3D accell;
  VectorFloat gravityVector;

  UserModMPU6050() :SysModule("Motion Tracking") {
    isEnabled = false; //need to enable after fresh setup
  };

  void setup() {
    SysModule::setup();
    parentVar = ui->initUserMod(parentVar, name, 6305);

    ui->initCheckBox(parentVar, "mtReady", &motionTrackingReady, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "tracking ready");
        return true;
      default: return false;
    }}); 

    ui->initCoord3D(parentVar, "gyro", &gyro, 0, UINT16_MAX, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setComment(var, "in degrees");
        return true;
      default: return false;
    }});

    ui->initCoord3D(parentVar, "accell", &accell, 0, UINT16_MAX, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setComment(var, "in m/s²");
        return true;
      default: return false;
    }}); 

    if (pinsM->initI2S()) {
      mpu.initialize();

      // verify connection
      if (mpu.testConnection()) {
        ppf("MPU6050 connection successful Initializing DMP...\n");
        uint8_t devStatus = mpu.dmpInitialize();

        if (devStatus == 0) {
          // // Calibration Time: generate offsets and calibrate our MPU6050
          mpu.CalibrateAccel(6);
          mpu.CalibrateGyro(6);
          // mpu.PrintActiveOffsets();
          
          mpu.setDMPEnabled(true); //mandatory

          // mpuIntStatus = mpu.getIntStatus();

          motionTrackingReady = true;
        }
        else {
          // ERROR!
          // 1 = initial memory load failed
          // 2 = DMP configuration updates failed
          // (if it's going to break, usually the code will be 1)
          ppf("DMP Initialization failed (code %d)\n", devStatus);
        }
      }
      else
        ppf("Testing device connections MPU6050 connection failed\n");
    }

    mdl->setValue("mtReady", motionTrackingReady);
  }

  void loop20ms() { // loop(): 700/s, loop20ms: 3000/s, loop1s(): 5500/s, disabled: 6000/s
    // mpu.getMotion6(&accell.x, &accell.y, &accell.z, &gyro.x, &gyro.y, &gyro.z);
    // // display tab-separated accel/gyro x/y/z values
    // ppf("mpu6050 %d,%d,%d %d,%d,%d\n", accell.x, accell.y, accell.z, gyro.x, gyro.y, gyro.z);

    // if programming failed, don't try to do anything
    if (!motionTrackingReady) return;
    // read a packet from FIFO
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) { // Get the Latest packet 
      mpu.dmpGetQuaternion(&q, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
      gyro.y = ypr[0] * 180/M_PI; //pan = yaw !
      gyro.x = ypr[1] * 180/M_PI; //tilt = pitch !
      gyro.z = ypr[2] * 180/M_PI; //roll = roll
      gravityVector = gravity;
      // display real acceleration, adjusted to remove gravity

      //needed to repeat the following 3 lines (yes if you look at the output: otherwise not 0)
      mpu.dmpGetQuaternion(&q, fifoBuffer); 
      mpu.dmpGetAccel(&aa, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);

      mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
      // mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q); //worked in 0.6.0, not in 1.3.0 anymore

      accell.x = aaReal.x;
      accell.y = aaReal.y;
      accell.z = aaReal.z;
    }
  }

  void loop1s() {
    //for debugging
    // ppf("mpu6050 ptr:%d,%d,%d ar:%d,%d,%d\n", gyro.x, gyro.y, gyro.z, accell.x, accell.y, accell.z);

    mdl->setValue("gyro", gyro);
    mdl->setValue("accell", accell);
  }

  private:
    MPU6050 mpu;

    // MPU control/status vars
    uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
    uint8_t fifoBuffer[64]; // FIFO storage buffer

    // orientation/motion vars
    Quaternion q;           // [w, x, y, z]         quaternion container
    VectorInt16 aa;         // [x, y, z]            accel sensor measurements
    VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
    // VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
    VectorFloat gravity;    // [x, y, z]            gravity vector
    // float euler[3];         // [psi, theta, phi]    Euler angle container
    float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

};

extern UserModMPU6050 *mpu6050;