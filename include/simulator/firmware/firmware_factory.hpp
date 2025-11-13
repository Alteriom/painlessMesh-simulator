/**
 * @file firmware_factory.hpp
 * @brief Factory for creating firmware instances
 * 
 * This file contains the FirmwareFactory class which manages firmware
 * registration and creation.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_FIRMWARE_FACTORY_HPP
#define SIMULATOR_FIRMWARE_FACTORY_HPP

#include "firmware_base.hpp"
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <iostream>

namespace simulator {
namespace firmware {

/**
 * @brief Factory class for creating firmware instances
 * 
 * FirmwareFactory uses the singleton pattern to provide a centralized
 * registry for firmware types. Firmware implementations can register
 * themselves with the factory, and nodes can create instances by name.
 * 
 * Example usage:
 * @code
 * // Register firmware (typically in firmware implementation file)
 * FirmwareFactory::instance().registerFirmware("SensorNode", 
 *   []() { return std::make_unique<SensorFirmware>(); });
 * 
 * // Create firmware instance by name
 * auto firmware = FirmwareFactory::instance().create("SensorNode");
 * if (firmware) {
 *   // Use firmware
 * }
 * @endcode
 */
class FirmwareFactory {
public:
  /// Firmware creator function type
  using Creator = std::function<std::unique_ptr<FirmwareBase>()>;
  
  /**
   * @brief Gets the singleton instance
   * 
   * @return Reference to the factory instance
   */
  static FirmwareFactory& instance() {
    static FirmwareFactory factory;
    return factory;
  }
  
  /**
   * @brief Registers a firmware type
   * 
   * @param name Firmware name/identifier
   * @param creator Function that creates firmware instances
   * @return true if registered successfully, false if name already exists
   * 
   * Example:
   * @code
   * FirmwareFactory::instance().registerFirmware("MyFirmware",
   *   []() { return std::make_unique<MyFirmware>(); });
   * @endcode
   */
  bool registerFirmware(const std::string& name, Creator creator) {
    if (creators_.find(name) != creators_.end()) {
      std::cerr << "[WARNING] Firmware '" << name 
                << "' is already registered" << std::endl;
      return false;
    }
    creators_[name] = creator;
    std::cout << "[INFO] Registered firmware: " << name << std::endl;
    return true;
  }
  
  /**
   * @brief Creates a firmware instance by name
   * 
   * @param name Firmware name/identifier
   * @return Unique pointer to firmware instance, or nullptr if not found
   * 
   * Example:
   * @code
   * auto firmware = FirmwareFactory::instance().create("SensorNode");
   * if (firmware) {
   *   node->loadFirmware(std::move(firmware));
   * }
   * @endcode
   */
  std::unique_ptr<FirmwareBase> create(const std::string& name) {
    auto it = creators_.find(name);
    if (it == creators_.end()) {
      std::cerr << "[ERROR] Unknown firmware: " << name << std::endl;
      std::cerr << "[INFO] Available firmware: ";
      bool first = true;
      for (const auto& pair : creators_) {
        if (!first) std::cerr << ", ";
        std::cerr << pair.first;
        first = false;
      }
      std::cerr << std::endl;
      return nullptr;
    }
    return it->second();
  }
  
  /**
   * @brief Checks if a firmware type is registered
   * 
   * @param name Firmware name/identifier
   * @return true if registered, false otherwise
   */
  bool isRegistered(const std::string& name) const {
    return creators_.find(name) != creators_.end();
  }
  
  /**
   * @brief Gets list of registered firmware names
   * 
   * @return Vector of registered firmware names
   */
  std::vector<std::string> getRegisteredNames() const {
    std::vector<std::string> names;
    names.reserve(creators_.size());
    for (const auto& pair : creators_) {
      names.push_back(pair.first);
    }
    return names;
  }
  
  /**
   * @brief Unregisters a firmware type
   * 
   * @param name Firmware name/identifier
   * @return true if unregistered, false if not found
   * 
   * This is mainly useful for testing.
   */
  bool unregisterFirmware(const std::string& name) {
    return creators_.erase(name) > 0;
  }
  
  /**
   * @brief Clears all registered firmware
   * 
   * This is mainly useful for testing.
   */
  void clear() {
    creators_.clear();
  }

private:
  /// Private constructor (singleton pattern)
  FirmwareFactory() = default;
  
  /// Prevent copying
  FirmwareFactory(const FirmwareFactory&) = delete;
  FirmwareFactory& operator=(const FirmwareFactory&) = delete;
  
  /// Registry of firmware creators
  std::map<std::string, Creator> creators_;
};

/**
 * @brief Helper macro for registering firmware
 * 
 * Use this macro in your firmware implementation file to automatically
 * register the firmware with the factory.
 * 
 * Example:
 * @code
 * // In my_firmware.cpp
 * REGISTER_FIRMWARE(SensorNode, SensorFirmware)
 * @endcode
 */
#define REGISTER_FIRMWARE(name, type) \
  namespace { \
    struct type##Registrar { \
      type##Registrar() { \
        simulator::firmware::FirmwareFactory::instance().registerFirmware( \
          #name, []() { return std::make_unique<type>(); }); \
      } \
    }; \
    static type##Registrar type##_registrar_instance; \
  }

} // namespace firmware
} // namespace simulator

#endif // SIMULATOR_FIRMWARE_FACTORY_HPP
