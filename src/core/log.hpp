#pragma once

#include "magic_numbers.hpp"

#include <sstream>
#include <iostream>

namespace home_task::log {

template <typename ... _Args>
void Write(_Args&&... _args) {
    if (!magic_numbers::WithLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}

template <typename ... _Args>
void WriteMutex(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithMutexLog || !magic_numbers::WithNetworkLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}

template <typename ... _Args>
void WriteNetwork(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithNetworkLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}


template <typename ... _Args>
void WriteState(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithStateLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}

template <typename ... _Args>
void WriteDestructor(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithDestructorLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}

}
