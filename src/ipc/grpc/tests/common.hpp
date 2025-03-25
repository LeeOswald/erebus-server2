#pragma once

#include <gtest/gtest.h>


#include <erebus/system/logger2.hxx>

#include <chrono>
#include <iostream>
#include <mutex>


extern std::uint16_t g_serverPort;
extern std::chrono::milliseconds g_operationTimeout;