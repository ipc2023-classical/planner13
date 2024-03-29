#include "heuristic.h"

#include "evaluation_context.h"
#include "evaluation_result.h"
#include "globals.h"
#include "option_parser.h"
#include "plugin.h"

#include "task_representation/state.h"
#include "task_representation/fts_task.h"
#include "task_transformation/task_transformation.h"
#include "task_transformation/state_mapping.h"

#include <cassert>
#include <cstdlib>
#include <limits>

using namespace std;
using namespace task_representation;

Heuristic::Heuristic(const Options &opts)
    : description(opts.get_unparsed_config()),
      heuristic_cache(HEntry(NO_VALUE, true)), //TODO: is true really a good idea here?
      cache_h_values(opts.get<bool>("cache_estimates")),
      cost_type(static_cast<OperatorCost>(opts.get_enum("cost_type"))) {
    auto transformation_method =
        opts.get<shared_ptr<task_transformation::TaskTransformation>> ("transform");

    
    auto transformation = transformation_method->transform_task_lossy(g_main_task);
    task = transformation.first;
    mapping = transformation.second;
    search_task = task->get_search_task(true);

    cout << "Heuristic task: " <<  *task << endl;
}

Heuristic::~Heuristic() {
}

 void Heuristic::set_preferred(int label, const FactPair & fact) {
     preferred_operators.set_preferred(label, fact);
}

bool Heuristic::notify_state_transition(
    const GlobalState & /*parent_state*/,
    const OperatorID /*op*/,
    const GlobalState & /*state*/) {
    return false;
}

State Heuristic::convert_global_state(const GlobalState &global_state) const {
    if (mapping.state_mapping) {
        return State(*task, mapping.state_mapping->convert_state(global_state));
    }else {
        return State(*task, global_state.get_values());
    }
}

void Heuristic::add_options_to_parser(OptionParser &parser) {
    parser.add_option<shared_ptr<task_transformation::TaskTransformation>>(
        "transform",
        "Optional task transformation for the heuristic.",
        "none()");
    parser.add_option<bool>("cache_estimates", "cache heuristic estimates", "true");

    add_cost_type_option_to_parser(parser);
}

// This solution to get default values seems nonoptimal.
// This is currently only used by the LAMA/FF synergy.
Options Heuristic::default_options() {
    Options opts = Options();
    opts.set<bool>("cache_estimates", false);
    return opts;
}
int Heuristic::get_label_cost(int label) const {
    return get_adjusted_action_cost(task->get_label_cost(label), cost_type);
}
    
EvaluationResult Heuristic::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;

    assert(preferred_operators.empty());

    const GlobalState &state = eval_context.get_state();
    bool calculate_preferred = eval_context.get_calculate_preferred();

    int heuristic = NO_VALUE;

    if (!calculate_preferred && cache_h_values &&
        heuristic_cache[state].h != NO_VALUE && !heuristic_cache[state].dirty) {
        heuristic = heuristic_cache[state].h;
        result.set_count_evaluation(false);
    } else {
        heuristic = compute_heuristic(state);
    
        if (cache_h_values) {
            heuristic_cache[state] = HEntry(heuristic, false);
        }
        result.set_count_evaluation(true);
    }

    assert(heuristic == DEAD_END || heuristic >= 0);

    if (heuristic == DEAD_END) {
        /*
          It is permissible to mark preferred operators for dead-end
          states (thus allowing a heuristic to mark them on-the-fly
          before knowing the final result), but if it turns out we
          have a dead end, we don't want to actually report any
          preferred operators.
        */
        preferred_operators.clear();
        heuristic = EvaluationResult::INFTY;
    }

// #ifndef NDEBUG
//     TaskProxy global_task_proxy = TaskProxy(*g_root_task());
//     State global_state(*g_root_task(), state.get_values());
//     OperatorsProxy global_operators = global_task_proxy.get_operators();
//     if (heuristic != EvaluationResult::INFTY) {
//         for (OperatorID op_id : preferred_operators)
//             assert(task_properties::is_applicable(global_operators[op_id], global_state));
//     }
// #endif

    result.set_h_value(heuristic);
    result.set_preferred_operators(std::move(preferred_operators));
    assert(preferred_operators.empty());

    return result;
}

string Heuristic::get_description() const {
    return description;
}


static PluginTypePlugin<Heuristic> _type_plugin(
    "Heuristic",
    "A heuristic specification is either a newly created heuristic "
    "instance or a heuristic that has been defined previously. "
    "This page describes how one can specify a new heuristic instance. "
    "For re-using heuristics, see OptionSyntax#Heuristic_Predefinitions.\n\n"
    "Definitions of //properties// in the descriptions below:\n\n"
    " * **admissible:** h(s) <= h*(s) for all states s\n"
    " * **consistent:** h(s) <= c(s, s') + h(s') for all states s "
    "connected to states s' by an action with cost c(s, s')\n"
    " * **safe:** h(s) = infinity is only true for states "
    "with h*(s) = infinity\n"
    " * **preferred operators:** this heuristic identifies "
    "preferred operators ");
