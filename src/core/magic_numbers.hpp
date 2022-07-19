#pragma once

#include <cstdint>


namespace home_task::magic_numbers {

constexpr uint64_t ServerId = 0;

constexpr bool WithSizeCalculation = true;

constexpr bool WithLog = true;
constexpr bool WithMutexLog = false;
constexpr bool WithNetworkLog = false;
constexpr bool WithStateLog = false;
constexpr bool WithDestructorLog = false;

}
