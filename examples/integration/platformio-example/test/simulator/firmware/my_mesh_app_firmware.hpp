#pragma once

#include "simulator/firmware/firmware_base.hpp"
#include <MyMeshApp.h>

namespace simulator {
namespace firmware {

/**
 * @brief Simulator adapter for MyMeshApp
 * 
 * This class wraps the MyMeshApp application for use in the simulator.
 * It extends FirmwareBase and delegates all callbacks to the application instance.
 * 
 * This is the ONLY simulator-specific code needed. Your application code
 * (MyMeshApp) remains platform-agnostic and works on both ESP32 and simulator.
 */
class MyMeshAppFirmware : public FirmwareBase {
public:
  MyMeshAppFirmware() : FirmwareBase("MyMeshApp") {}
  
  void setup() override {
    // Create application instance with simulator's mesh and scheduler
    app_ = std::make_unique<MyMeshApp>(mesh_, scheduler_);
    app_->setup();
  }
  
  void loop() override {
    app_->loop();
  }
  
  void onReceive(uint32_t from, String& msg) override {
    app_->onReceive(from, msg);
  }
  
  void onNewConnection(uint32_t nodeId) override {
    app_->onNewConnection(nodeId);
  }
  
  void onChangedConnections() override {
    app_->onChangedConnections();
  }

private:
  std::unique_ptr<MyMeshApp> app_;
};

} // namespace firmware
} // namespace simulator
