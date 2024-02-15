cmake_minimum_required(VERSION 3.5)
project(roomba_500driver_meiji)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

set(msg_files
  "msg/Button.msg"
  "msg/LeftRight16.msg"
  "msg/Roomba500State.msg"
  "msg/Cliff.msg"
  "msg/LeftRightU16.msg"
  "msg/RoombaCtrl.msg"
  "msg/IrOpcode.msg"
  "msg/LightBumper.msg"
  "msg/Song.msg"
  "msg/LeftRight.msg"
  "msg/MotorOvercurrent.msg"
  "msg/Wheeldrop.msg"
)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Button.msg"
  "msg/LeftRight16.msg"
  "msg/Roomba500State.msg"
  "msg/Cliff.msg"
  "msg/LeftRightU16.msg"
  "msg/RoombaCtrl.msg"
  "msg/IrOpcode.msg"
  "msg/LightBumper.msg"
  "msg/Song.msg"
  "msg/LeftRight.msg"
  "msg/MotorOvercurrent.msg"
  "msg/Wheeldrop.msg"
  DEPENDENCIES geometry_msgs std_msgs
  ADD_LINTER_TESTS
)
ament_export_dependencies(rosidl_default_runtime)

# This necessary when generating messages and using them in the same package
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp
)

ament_auto_add_library(libroomba_500driver_meiji src/main500.cpp src/roomba500sci.cpp src/serial.cpp)
target_link_libraries(
  libroomba_500driver_meiji "${cpp_typesupport_target}"
)
ament_auto_add_executable(main500 src/main500.cpp)
ament_auto_add_executable(roomba500sci src/roomba500sci.cpp)
ament_auto_add_executable(serial src/serial.cpp)
ament_auto_add_executable(twist_to_roombactrl_converter src/twist_to_roombactrl_converter.cpp)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_auto_package(
  CONFIG_EXTRAS "roomba_500driver_meiji-extras.cmake"
)
