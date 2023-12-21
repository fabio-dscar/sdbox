#ifndef __SDBOX_H__
#define __SDBOX_H__

#include <cstdlib>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <format>
#include <filesystem>
#include <map>

#if defined(DEBUG)
#define THROW_ERROR(...)                                                                 \
    throw std::runtime_error(std::format("{} ({}): {}", std::string(__FILE__),           \
                                         std::to_string(__LINE__),                       \
                                         std::format(__VA_ARGS__)))
#else
#define THROW_ERROR(...) throw std::runtime_error(std::format(__VA_ARGS__))
#endif

#define FATAL(...) THROW_ERROR(__VA_ARGS__)

#endif // __SDBOX_H__