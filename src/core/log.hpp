#pragma once

#include "magic_numbers.hpp"

#include <sstream>
#include <iostream>

namespace home_task::log {

template <bool ... _Flags>
struct Logger {
    template <typename ... _Args>
    static void Write(_Args&&... _args) {
        if ((false || ... || _Flags)) {
            std::ostringstream sout;
            (sout << ... << std::forward<_Args>(_args)) << std::endl;
            std::cerr << sout.str();
        }
    }
};

template <typename ... _Args>
void ForceWrite(_Args&&... _args) {
    Logger<true>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void Write(_Args&&... _args) {
    Logger<magic_numbers::WithLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteInit(_Args&&... _args) {
    Logger<magic_numbers::WithInitLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteFullStateLog(_Args&&... _args) {
    Logger<magic_numbers::WithFullStateLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteHistoryLog(_Args&&... _args) {
    Logger<magic_numbers::WithHistoryLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteMutex(_Args&&... _args) {
    Logger<magic_numbers::WithMutexLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteNetwork(_Args&&... _args) {
    Logger<magic_numbers::WithNetworkLog>::Write(std::forward<_Args>(_args)...);
}


template <typename ... _Args>
void WriteState(_Args&&... _args) {
    Logger<magic_numbers::WithStateLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteServerState(_Args&&... _args) {
    Logger<magic_numbers::WithStateLog, magic_numbers::WithServerStateLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteClientState(_Args&&... _args) {
    Logger<magic_numbers::WithStateLog, magic_numbers::WithClientStateLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteFastClientState(_Args&&... _args) {
    Logger<magic_numbers::WithStateLog, magic_numbers::WithClientStateLog, magic_numbers::WithFastClientStateLog>
        ::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteDestructor(_Args&&... _args) {
    Logger<magic_numbers::WithDestructorLog>::Write(std::forward<_Args>(_args)...);
}


template <typename ... _Args>
void WriteClientRunner(_Args&&... _args) {
    Logger<magic_numbers::WithRunnerLog, magic_numbers::WithClientRunnerLog>::Write(std::forward<_Args>(_args)...);
}


template <typename ... _Args>
void WriteServerRunner(_Args&&... _args) {
    Logger<magic_numbers::WithRunnerLog, magic_numbers::WithServerRunnerLog>::Write(std::forward<_Args>(_args)...);
}

template <typename ... _Args>
void WriteDecardTree(_Args&&... _args) {
    Logger<magic_numbers::WithDecardTreeLog>::Write(std::forward<_Args>(_args)...);
}

}
