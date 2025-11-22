/**
 * @file ino_firmware_wrapper.hpp
 * @brief Wrapper for loading Arduino .ino examples as firmware
 * 
 * This file provides infrastructure to load painlessMesh .ino examples
 * as firmware in the simulator. This enables testing real-world firmware
 * code without requiring physical ESP32/ESP8266 hardware.
 * 
 * Purpose:
 * - Test painlessMesh library with actual user code (.ino examples)
 * - Catch bugs that only manifest with specific firmware patterns
 * - Validate library changes don't break existing examples
 * - Provide regression testing for reported issues
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_INO_FIRMWARE_WRAPPER_HPP
#define SIMULATOR_INO_FIRMWARE_WRAPPER_HPP

#include "firmware_base.hpp"
#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"
#include "Arduino.h"
#include <functional>
#include <memory>

namespace simulator {
namespace firmware {

/**
 * @brief Interface for .ino firmware implementations
 * 
 * .ino files typically define these functions:
 * - void setup()
 * - void loop()
 * - void receivedCallback(uint32_t from, String& msg)
 * - void newConnectionCallback(uint32_t nodeId)
 * - void changedConnectionCallback()
 * - void nodeTimeAdjustedCallback(int32_t offset)
 * 
 * This interface allows us to wrap them in a FirmwareBase implementation.
 */
struct InoFirmwareInterface {
  std::function<void()> setup;
  std::function<void()> loop;
  std::function<void(uint32_t, String&)> receivedCallback;
  std::function<void(uint32_t)> newConnectionCallback;
  std::function<void()> changedConnectionCallback;
  std::function<void(int32_t)> nodeTimeAdjustedCallback;
};

/**
 * @brief Base class for wrapping .ino examples as firmware
 * 
 * This class wraps Arduino .ino sketch functions into the FirmwareBase
 * interface so they can run in the simulator.
 * 
 * Usage:
 * 1. Create a subclass for your .ino example
 * 2. Implement createInoInterface() to bind .ino functions
 * 3. Register the firmware with FIRMWARE_REGISTER()
 * 
 * Example:
 * @code
 * class BasicInoFirmware : public InoFirmwareWrapper {
 * public:
 *   BasicInoFirmware() : InoFirmwareWrapper("basic.ino") {}
 * 
 * protected:
 *   InoFirmwareInterface createInoInterface() override {
 *     InoFirmwareInterface iface;
 *     iface.setup = std::bind(&BasicInoFirmware::ino_setup, this);
 *     iface.loop = std::bind(&BasicInoFirmware::ino_loop, this);
 *     iface.receivedCallback = std::bind(&BasicInoFirmware::ino_receivedCallback, 
 *                                        this, std::placeholders::_1, std::placeholders::_2);
 *     return iface;
 *   }
 * 
 * private:
 *   // .ino functions
 *   void ino_setup() {}
 *   void ino_loop() {}
 *   void ino_receivedCallback(uint32_t from, String& msg) {}
 * };
 * (end code example)
 */
class InoFirmwareWrapper : public FirmwareBase {
public:
  /**
   * @brief Constructor
   * 
   * @param ino_name Name of the .ino file (for identification)
   */
  explicit InoFirmwareWrapper(const std::string& ino_name)
    : FirmwareBase(ino_name), ino_name_(ino_name) {}
  
  /**
   * @brief Setup firmware (calls .ino setup())
   */
  void setup() override {
    if (!ino_interface_created_) {
      ino_interface_ = createInoInterface();
      ino_interface_created_ = true;
    }
    
    if (ino_interface_.setup) {
      ino_interface_.setup();
    }
  }
  
  /**
   * @brief Main loop (calls .ino loop())
   */
  void loop() override {
    if (ino_interface_.loop) {
      ino_interface_.loop();
    }
  }
  
  /**
   * @brief Message received callback
   */
  void onReceive(uint32_t from, String& msg) override {
    if (ino_interface_.receivedCallback) {
      ino_interface_.receivedCallback(from, msg);
    }
  }
  
  /**
   * @brief New connection callback
   */
  void onNewConnection(uint32_t nodeId) override {
    if (ino_interface_.newConnectionCallback) {
      ino_interface_.newConnectionCallback(nodeId);
    }
  }
  
  /**
   * @brief Topology changed callback
   */
  void onChangedConnections() override {
    if (ino_interface_.changedConnectionCallback) {
      ino_interface_.changedConnectionCallback();
    }
  }
  
  /**
   * @brief Time adjusted callback
   */
  void onNodeTimeAdjusted(int32_t offset) override {
    if (ino_interface_.nodeTimeAdjustedCallback) {
      ino_interface_.nodeTimeAdjustedCallback(offset);
    }
  }
  
  /**
   * @brief Get the .ino filename
   * @return .ino filename
   */
  std::string getInoName() const { return ino_name_; }

protected:
  /**
   * @brief Create the .ino interface binding
   * 
   * Override this method to bind your .ino functions to the interface.
   * 
   * @return Interface with bound .ino functions
   */
  virtual InoFirmwareInterface createInoInterface() = 0;

public:
  /**
   * @brief Get mesh instance for .ino code
   * 
   * .ino files typically have a global `mesh` variable. This helper
   * allows subclasses to access the mesh in their .ino functions.
   * 
   * @return Pointer to mesh instance
   */
  painlessmesh::Mesh<painlessmesh::Connection>* getMesh() {
    return mesh_;
  }
  
  /**
   * @brief Get scheduler instance for .ino code
   * 
   * .ino files typically have a global `userScheduler` variable.
   * 
   * @return Pointer to scheduler instance
   */
  Scheduler* getScheduler() {
    return scheduler_;
  }

private:
  std::string ino_name_;
  InoFirmwareInterface ino_interface_;
  bool ino_interface_created_{false};
};

} // namespace firmware
} // namespace simulator

#endif // SIMULATOR_INO_FIRMWARE_WRAPPER_HPP
