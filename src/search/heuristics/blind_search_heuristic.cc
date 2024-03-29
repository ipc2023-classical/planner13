#include "blind_search_heuristic.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_representation/search_task.h"

#include <cstddef>
#include <limits>
#include <utility>

using namespace std;

namespace blind_search_heuristic {
BlindSearchHeuristic::BlindSearchHeuristic(const Options &opts)
    : Heuristic(opts),
      min_operator_cost(task->get_min_operator_cost()) {
    cout << "Initializing blind search heuristic..." << endl;
}

BlindSearchHeuristic::~BlindSearchHeuristic() {
}

int BlindSearchHeuristic::compute_heuristic(const GlobalState & state) {
    if (task->is_goal_state(state))
        return 0;
    else
        return min_operator_cost;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Blind heuristic",
                             "Returns cost of cheapest action for "
                             "non-goal states, "
                             "0 for goal states");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support("axioms", "supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new BlindSearchHeuristic(opts);
}

static Plugin<Heuristic> _plugin("blind", _parse);
}
