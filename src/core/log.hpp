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
void WriteServerState(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithStateLog || !magic_numbers::WithServerStateLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}

template <typename ... _Args>
void WriteClientState(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithStateLog || !magic_numbers::WithClientStateLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}

template <typename ... _Args>
void WriteFastClientState(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithStateLog || !magic_numbers::WithFastClientStateLog) {
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


template <typename ... _Args>
void WriteClientRunner(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithRunnerLog || !magic_numbers::WithClientRunnerLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}


template <typename ... _Args>
void WriteServerRunner(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithRunnerLog || !magic_numbers::WithServerRunnerLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}

template <typename ... _Args>
void WriteDecardTree(_Args&&... _args) {
    if (!magic_numbers::WithLog || !magic_numbers::WithDecardTreeLog) {
        return;
    }
    std::ostringstream sout;
    (sout << ... << std::forward<_Args>(_args)) << std::endl;
    std::cerr << sout.str();
}

}
