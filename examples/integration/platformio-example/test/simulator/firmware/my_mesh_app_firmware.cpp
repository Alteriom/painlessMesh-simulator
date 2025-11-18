#include "my_mesh_app_firmware.hpp"
#include "simulator/firmware/firmware_factory.hpp"

using namespace simulator::firmware;

// Auto-register firmware with the simulator's firmware factory
// This makes "MyMeshApp" available as a firmware type in YAML scenarios
REGISTER_FIRMWARE(MyMeshApp, MyMeshAppFirmware)
