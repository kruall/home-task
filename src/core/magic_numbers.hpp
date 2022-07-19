#pragma once

#include <cstdint>


namespace home_task::magic_numbers {

constexpr uint64_t ServerId = 0;

constexpr bool WithSizeCalculation = true;
constexpr bool CalculateFirstLoadState = false;
constexpr bool WithStateChecking = false;

constexpr bool WithLog = true;
constexpr bool WithMutexLog = false;
constexpr bool WithNetworkLog = false;
constexpr bool WithStateLog = false;
constexpr bool WithServerStateLog = false;
constexpr bool WithClientStateLog = false;
constexpr bool WithFastClientStateLog = false;
constexpr bool WithDestructorLog = false;
constexpr bool WithRunnerLog = false;
constexpr bool WithServerRunnerLog = false;
constexpr bool WithClientRunnerLog = false;
constexpr bool WithDecardTreeLog = false;

constexpr bool UserSendLoadState = false;
constexpr bool UserSendSync = true;

}
