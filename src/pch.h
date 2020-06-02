#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#define WIN32_LEAN_AND_MEAN

#ifdef _MSC_VER
#ifdef NDEBUG
#define _SECURE_SCL 0
#define HAS_ITERATOR_DEBUGGING 0
#endif

#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance
#pragma warning(disable:4267) // 'var' : conversion from 'uint32_t' to 'Type', possible loss of data
#pragma warning(disable:4351) // new behavior: elements of array will be default initialized
#pragma warning(disable:4458) // declaration hides class member
#endif

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#ifndef _WIN32_WINNT
// 0x0602: Windows 7
#define _WIN32_WINNT 0x0602
#endif
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <fmt/printf.h>
#include <fmt/format.h>

#include <boost/asio.hpp>
