cmake_minimum_required(VERSION 3.5)

project(tensor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tensor main.cpp tensor.cpp tensor.h)
