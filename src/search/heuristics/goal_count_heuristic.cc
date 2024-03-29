#include "goal_count_heuristic.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <iostream>
using namespace std;

namespace goal_count_heuristic {
GoalCountHeuristic::GoalCountHeuristic(const Options &opts)
    : Heuristic(opts) {
    cout << "Initializing goal count heuristic..." << endl;
}

GoalCountHeuristic::~GoalCountHeuristic() {
}

int GoalCountHeuristic::compute_heuristic(const GlobalState &global_state) {
    auto state = convert_global_state(global_state);
    if (state.is_dead_end()) {
        return DEAD_END;
    }
    int unsatisfied_goal_count = 0;

    for (FactProxy goal : task_proxy.get_goals()) {
        const VariableProxy var = goal.get_variable();
        if (state[var] != goal) {
            ++unsatisfied_goal_count;
        }
    }
    return unsatisfied_goal_count;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Goal count heuristic", "");
    parser.document_language_support("action costs", "ignored by design");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support("axioms", "supported");
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new GoalCountHeuristic(opts);
}


static Plugin<Heuristic> _plugin("goalcount", _parse);
}
