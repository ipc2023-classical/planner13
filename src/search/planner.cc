#include "option_parser.h"
#include "search_engine.h"

#include "utils/system.h"
#include "utils/timer.h"
#include "utils/logging.h"

#include "task_representation/sas_task.h"
#include "task_transformation/task_transformation_method.h"

#include <iostream>

using namespace std;
using utils::ExitCode;

using task_transformation::TaskTransformationMethod;

int main(int argc, const char **argv) {
    utils::register_event_handlers();

    if (argc < 2) {
        cout << OptionParser::usage(argv[0]) << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }

    if (static_cast<string>(argv[1]) != "--help") {
        g_sas_task()->read_from_file(cin);
        g_log << "Main task constructed" << endl;
    }
    
    shared_ptr<TaskTransformationMethod> transformer;

    // The command line is parsed twice: once in dry-run mode, to check for simple input
    // errors, and then in normal mode.
    bool is_unit_cost = g_sas_task()->is_unit_cost();
    try {
        OptionParser::parse_cmd_line_transform(argc, argv, true, is_unit_cost);
        transformer = OptionParser::parse_cmd_line_transform(argc, argv, false, is_unit_cost);
    } catch (ArgError &error) {
        cerr << error << endl;
        OptionParser::usage(argv[0]);
        utils::exit_with(ExitCode::INPUT_ERROR);
    } catch (ParseError &error) {
        cerr << error << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }

    utils::Timer transform_timer;
    assert(transformer);

    cout << "Transform task... " << endl;
    auto transformation = transformer->transform_task(*g_sas_task());
    g_main_task = transformation.first;
    g_plan_reconstruction = transformation.second;
    //TODO is_unit_cost = g_main_task->is_unit_cost();
    g_log << "Transform time: " << transform_timer << endl;

    shared_ptr<SearchEngine> engine;

    // The command line is parsed twice: once in dry-run mode, to
    // check for simple input errors, and then in normal mode.
    try {
        OptionParser::parse_cmd_line(argc, argv, true, is_unit_cost);
        engine = OptionParser::parse_cmd_line(argc, argv, false, is_unit_cost);
    } catch (ArgError &error) {
        cerr << error << endl;
        OptionParser::usage(argv[0]);
        utils::exit_with(ExitCode::INPUT_ERROR);
    } catch (ParseError &error) {
        cerr << error << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }

    utils::Timer search_timer;
    g_log << "Start search." << endl;

    engine->search();
    search_timer.stop();
    utils::g_timer.stop();

    engine->save_plan_if_necessary();
    engine->print_statistics();
    cout << "Search time: " << search_timer << endl;
    cout << "Total time: " << utils::g_timer << endl;

    if (engine->found_solution()) {
        utils::exit_with(ExitCode::PLAN_FOUND);
    } else {
        utils::exit_with(ExitCode::UNSOLVED_INCOMPLETE);
    }
}
