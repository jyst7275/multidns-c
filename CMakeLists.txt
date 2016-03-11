cmake_minimum_required(VERSION 3.0)
project(multidns_c)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -L/usr/local/lib/ -lboost_regex -pthread")

set(SOURCE_FILES multidns.cpp dnsparser.h DnsDispatcher.h jsoncpp.cpp metadata.h)
add_executable(multidns_c ${SOURCE_FILES})
