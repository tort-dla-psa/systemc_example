cmake_minimum_required(VERSION 3.1)
project(SystemCExample)


find_package(SystemCLanguage CONFIG REQUIRED)
set (CMAKE_CXX_STANDARD ${SystemC_CXX_STANDARD})

add_executable(test_systemc main.cpp)
target_link_libraries(test_systemc SystemC::systemc)
