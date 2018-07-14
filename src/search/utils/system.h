#ifndef UTILS_SYSTEM_H
#define UTILS_SYSTEM_H

#define LINUX 0
#define OSX 1
#define WINDOWS 2

#if defined(_WIN32)
#define OPERATING_SYSTEM WINDOWS
#include "system_windows.h"
#elif defined(__APPLE__)
#define OPERATING_SYSTEM OSX
#include "system_unix.h"
#else
#define OPERATING_SYSTEM LINUX
#include "system_unix.h"
#endif

#include "language.h"

#include <iostream>

#define ABORT(msg) \
    ( \
        (std::cerr << "Critical error in file " << __FILE__ \
                   << ", line " << __LINE__ << ": " << std::endl \
                   << (msg) << std::endl), \
        (abort()), \
        (void)0 \
    )

namespace utils {
enum class ExitCode {
    PLAN_FOUND = 0,
    CRITICAL_ERROR = 1,
    INPUT_ERROR = 2,
    UNSUPPORTED = 3,
    // Task is provably unsolvable with given bound.
    UNSOLVABLE = 4,
    // Search ended without finding a solution.
    UNSOLVED_INCOMPLETE = 5,
    OUT_OF_MEMORY = 6
};

NO_RETURN extern void exit_with(ExitCode returncode);

int get_peak_memory_in_kb();
const char *get_exit_code_message_reentrant(ExitCode exitcode);
bool is_exit_code_error_reentrant(ExitCode exitcode);
void register_event_handlers();
void report_exit_code_reentrant(ExitCode exitcode);
int get_process_id();


//TODO: Move this where it belongs
void check_magic(std::istream &in, std::string magic);
void read_and_verify_version(std::istream &in);

}

#endif
