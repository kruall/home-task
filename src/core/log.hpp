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


}
