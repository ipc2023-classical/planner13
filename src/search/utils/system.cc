#include "system.h"

using namespace std;
namespace utils {
const char *get_exit_code_message_reentrant(ExitCode exitcode) {
    switch (exitcode) {
    case ExitCode::PLAN_FOUND:
        return "Solution found.";
    case ExitCode::CRITICAL_ERROR:
        return "Unexplained error occurred.";
    case ExitCode::INPUT_ERROR:
        return "Usage error occurred.";
    case ExitCode::UNSUPPORTED:
        return "Tried to use unsupported feature.";
    case ExitCode::UNSOLVABLE:
        return "Task is provably unsolvable.";
    case ExitCode::UNSOLVED_INCOMPLETE:
        return "Search stopped without finding a solution.";
    case ExitCode::OUT_OF_MEMORY:
        return "Memory limit has been reached.";
    default:
        return nullptr;
    }
}

bool is_exit_code_error_reentrant(ExitCode exitcode) {
    switch (exitcode) {
    case ExitCode::PLAN_FOUND:
    case ExitCode::UNSOLVABLE:
    case ExitCode::UNSOLVED_INCOMPLETE:
    case ExitCode::OUT_OF_MEMORY:
        return false;
    case ExitCode::CRITICAL_ERROR:
    case ExitCode::INPUT_ERROR:
    case ExitCode::UNSUPPORTED:
    default:
        return true;
    }
}

void exit_with(ExitCode exitcode) {
    report_exit_code_reentrant(exitcode);
    exit(static_cast<int>(exitcode));
}

    
void check_magic(istream &in, string magic) {
    string word;
    in >> word;
    if (word != magic) {
        cout << "Failed to match magic word '" << magic << "'." << endl;
        cout << "Got '" << word << "'." << endl;
        if (magic == "begin_version") {
            cerr << "Possible cause: you are running the planner "
                 << "on a preprocessor file from " << endl
                 << "an older version." << endl;
        }
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
}

    
static const int PRE_FILE_VERSION = 3;

void read_and_verify_version(istream &in) {
    int version;
    check_magic(in, "begin_version");
    in >> version;
    check_magic(in, "end_version");
    if (version != PRE_FILE_VERSION) {
        cerr << "Expected preprocessor file version " << PRE_FILE_VERSION
             << ", got " << version << "." << endl;
        cerr << "Exiting." << endl;
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
}

}
