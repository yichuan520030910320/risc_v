cmake_minimum_required(VERSION 3.10)
project(risc_v)

set(CMAKE_CXX_STANDARD 14)

add_executable(risc_v main.cpp stage.cpp stage.h enum.h)