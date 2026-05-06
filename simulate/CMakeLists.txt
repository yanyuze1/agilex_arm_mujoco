cmake_minimum_required(VERSION 3.16)
project(agilex_mujoco)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(AGILEX_PLUGIN_SRC)
option(AGILEX_ENABLE_ROS2_CONTROL "Build ROS2 control plugin" ON)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  message(STATUS "Build type: Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fPIC")
else()
  message(STATUS "Build type: Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -fPIC")
endif()

if(AGILEX_ENABLE_ROS2_CONTROL)
  find_package(ament_cmake REQUIRED)
  find_package(rclcpp REQUIRED)
  find_package(pluginlib REQUIRED)
  find_package(controller_manager REQUIRED)
  find_package(mujoco_sim_ros2 REQUIRED)

  list(APPEND AGILEX_PLUGIN_SRC src/ros2_control_plugin.cc)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(Threads REQUIRED)

include_directories(
  mujoco/include
  mujoco/simulate
  src
  src/lodepng
)

link_directories(
  mujoco/lib
)

file(GLOB SIM_SRC
  mujoco/simulate/glfw_*.cc
  mujoco/simulate/platform_*.cc
  mujoco/simulate/simulate.cc
  src/lodepng/lodepng.cpp
)

add_executable(agilex_mujoco
  ${SIM_SRC}
  ${AGILEX_PLUGIN_SRC}
  src/main.cc
  src/plugin_manager.cc
)

target_compile_definitions(agilex_mujoco PRIVATE
  AGILEX_ENABLE_ROS2_CONTROL=$<BOOL:${AGILEX_ENABLE_ROS2_CONTROL}>
)


target_link_libraries(agilex_mujoco
  pthread
  mujoco
  glfw
  yaml-cpp
  Boost::program_options
  fmt
  Threads::Threads
)
if(AGILEX_ENABLE_ROS2_CONTROL)
  ament_target_dependencies(agilex_mujoco
    rclcpp
    pluginlib
    controller_manager
    mujoco_sim_ros2
  )
endif()


set_target_properties(agilex_mujoco PROPERTIES
  BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/mujoco/lib"
  INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/mujoco/lib"
)
