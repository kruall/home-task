#pragma once

#include <cstdint>


namespace home_task::magic_numbers {

constexpr uint64_t ServerId = 0;

constexpr bool WithSizeCalculation = true;
constexpr bool CalculateFirstLoadState = true;
constexpr bool WithStateChecking = true;
constexpr bool WithDelayedHistory= false;

constexpr bool FastSwith = false;

constexpr bool WithLog = true;
constexpr bool WithMutexLog = false;
constexpr bool WithNetworkLog = false;
constexpr bool WithStateLog = FastSwith;
constexpr bool WithServerStateLog = false;
constexpr bool WithClientStateLog = false;
constexpr bool WithFastClientStateLog = false;
constexpr bool WithDestructorLog = false;
constexpr bool WithRunnerLog = false;
constexpr bool WithServerRunnerLog = false;
constexpr bool WithClientRunnerLog = false;
constexpr bool WithDecardTreeLog = false;
constexpr bool WithInitLog = false;
constexpr bool WithFullStateLog = FastSwith;
constexpr bool WithHistoryLog = FastSwith;

constexpr bool UserSendLoadState = true;
constexpr bool UserSendSync = true;

}
