cmake_minimum_required(VERSION 3.10)
project(MyMeshAppSimulatorTests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add painlessMesh-simulator
# Adjust path if simulator is in different location
set(SIMULATOR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." CACHE PATH "Path to simulator root")
add_subdirectory(${SIMULATOR_PATH} simulator)

# Your application library (shared between ESP32 and simulator)
add_library(my_mesh_app
  ../../lib/MyMeshApp/MyMeshApp.cpp
)

target_include_directories(my_mesh_app PUBLIC
  ../../lib
  ${PAINLESSMESH_PATH}/src
)

target_link_libraries(my_mesh_app
  PUBLIC
    painlessmesh_lib
)

# Simulator firmware adapter
add_library(my_mesh_app_firmware
  firmware/my_mesh_app_firmware.cpp
)

target_include_directories(my_mesh_app_firmware PUBLIC
  firmware
)

target_link_libraries(my_mesh_app_firmware
  PUBLIC
    my_mesh_app
    simulator_firmware
)

# Link firmware into simulator executable
target_link_libraries(painlessmesh-simulator
  PRIVATE
    my_mesh_app_firmware
)
