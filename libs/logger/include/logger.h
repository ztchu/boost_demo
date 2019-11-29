#pragma once

#define LOG_CONSOLE

#ifdef LOG_CONSOLE
#include <iostream>

#define LOG_INFO std::cout
#define LOG_WARN std::cerr
#define LOG_ERROR std::cerr
#define ENDLINE std::endl
#else

#define LOG_ENDLINE ""

#endif