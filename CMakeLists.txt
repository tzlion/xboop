cmake_minimum_required(VERSION 3.27)
project(xboop)

set(CMAKE_CXX_STANDARD 11)

add_executable(xboop main.c
        ppgb.c
        ppgb.h)
