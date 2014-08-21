#include "blind_search_heuristic.h"

#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "task.h"

#include <limits>
#include <utility>
using namespace std;

BlindSearchHeuristic::BlindSearchHeuristic(const Options &opts)
    : Heuristic(opts) {
    min_operator_cost = numeric_limits<int>::max();
    Operators ops = task.get_operators();
    size_t num_operators = ops.size();
    for (size_t i = 0; i < num_operators; ++i)
        min_operator_cost = min(min_operator_cost,
                                ops[i].get_adjusted_cost(cost_type));
}

BlindSearchHeuristic::~BlindSearchHeuristic() {
}

void BlindSearchHeuristic::initialize() {
    cout << "Initializing blind search heuristic..." << endl;
}

int BlindSearchHeuristic::compute_heuristic(const State &state) {
    if (test_goal(state))
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
