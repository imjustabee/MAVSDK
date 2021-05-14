cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(radio_calibrate)

find_package(Threads REQUIRED)

if(NOT MSVC)
    add_definitions("-Wall -Wextra")
else()
    add_definitions("-WX -W2")
endif()

find_package(MAVSDK REQUIRED)

add_executable(radio_calibrate
    radio_calibrate.cpp
)

target_link_libraries(calibrate
    MAVSDK::mavsdk_radio_calibration
    MAVSDK::mavsdk
    ${CMAKE_THREAD_LIBS_INIT}
)

