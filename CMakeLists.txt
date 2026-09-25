cmake_minimum_required(VERSION 3.16)
project(McapViewer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2)

include(FetchContent)
FetchContent_Declare(
  mcap
  GIT_REPOSITORY https://github.com/foxglove/mcap.git
  GIT_TAG        releases/cpp/v1.4.0
)
FetchContent_GetProperties(mcap)
if(NOT mcap_POPULATED)
  FetchContent_Populate(mcap)
endif()

add_executable(McapViewer
  src/main.cpp
  src/mcap_impl.cpp
  src/McapController.cpp
  src/McapImageProvider.cpp
  qml/qml.qrc
)

target_include_directories(McapViewer PRIVATE
  src
  ${mcap_SOURCE_DIR}/cpp/mcap/include
)

target_compile_definitions(McapViewer PRIVATE
  MCAP_COMPRESSION_ZSTD
  MCAP_COMPRESSION_LZ4
)

target_link_libraries(McapViewer PRIVATE
  Qt5::Core
  Qt5::Gui
  Qt5::Quick
  Qt5::Qml
  Qt5::QuickControls2
  zstd
  lz4
)
