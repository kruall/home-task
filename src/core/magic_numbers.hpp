#pragma once

#include <cstdint>


namespace home_task::magic_numbers {

constexpr uint64_t ServerId = 0;

constexpr bool WithSizeCalculation = true;
constexpr bool WithStateChecking = false;

constexpr bool WithLog = true;
constexpr bool WithMutexLog = false;
constexpr bool WithNetworkLog = false;
constexpr bool WithStateLog = false;
constexpr bool WithServerStateLog = true;
constexpr bool WithClientStateLog = true;
constexpr bool WithFastClientStateLog = false;
constexpr bool WithDestructorLog = false;
constexpr bool WithRunnerLog = false;
constexpr bool WithServerRunnerLog = true;
constexpr bool WithClientRunnerLog = true;
constexpr bool WithDecardTreeLog = false;

constexpr bool UserSendLoadState = false;
constexpr bool UserSendSync = true;

}
