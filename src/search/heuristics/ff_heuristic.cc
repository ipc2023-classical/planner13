#include "ff_heuristic.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../task_representation/search_task.h"
#include "../task_representation/state.h"

#include <cassert>

using namespace std;

namespace ff_heuristic {
// construction and destruction
FFHeuristic::FFHeuristic(const Options &opts)
    : AdditiveHeuristic(opts),
      optimize_relaxed_plan (opts.get<bool>("optimize_relaxed_plan")) {
    cout << "Initializing FF heuristic..." << endl;
}

FFHeuristic::~FFHeuristic() {
}

void FFHeuristic::relaxed_plan_extraction(Proposition *goal) {
    if (!goal->marked) { // Only consider each subgoal once.
        goal->marked = true;
        UnaryOperator *unary_op = goal->reached_by;
        if (unary_op) { // We have not yet chained back to a start node.
            for (size_t i = 0; i < unary_op->precondition.size(); ++i)
                relaxed_plan_extraction(unary_op->precondition[i]);
	    if (unary_op->rp_step.label != -1) {
		relaxed_plan.push_back(unary_op->rp_step);
	    }
        }
    }
}

int FFHeuristic::compute_heuristic(const GlobalState &global_state) {
    task_representation::State state = convert_global_state(global_state);
    int h_add = compute_add_and_ff(state);
    if (h_add == DEAD_END)
        return h_add;

    // Relaxed plan extraction gives us a list of <l, fact> in relaxed_plan
    for (size_t i = 0; i < goal_propositions.size(); ++i) {
        relaxed_plan_extraction(goal_propositions[i]);
    }
    
    int h_ff = 0;

    if(optimize_relaxed_plan) {
	// for (size_t var = 0; var < propositions_per_var.size(); ++var) {
	//     propositions_per_var[var][global_state[var]].marked = false;
	// }

	// for (auto step = relaxed_plan.rbegin(); step != relaxed_plan.rend(); ++step) {
	//     int label = step->label;
	//     auto effect = step->effect;
	    
	//     if (propositions_per_var[effect.var][state[effect.value]].marked) {
	// 	//We need to apply some operator associated with l to achieve
	// 	const auto candidates = get_operators_achieve(step);
	// 	OperatorID op;
		
	// 	if (candidates.size() > 1) {
	// 	    for (OperatorID candidate_op : candidates) {
	// 		if(is_relaxed_applicable(candidate_op)) {
	// 		    op = candidate_op;
	// 		    break;
	// 		}
	// 	    }
	// 	} else {
	// 	    op = candidates[0];
	// 	}

	// 	propositions_per_var[effect.var][effect.value] = false;
        //         //TODO: Apply "static" effects of the operator
	       
	// 	h_ff += task->get_label_cost (label);


	// 	if (search_task->is_applicable(global_state, op)) {
	// 	    set_preferred(op);
	// 	}
	//     }
	// }
    } else {   
	for (const auto & step : relaxed_plan) {
	    h_ff += task->get_label_cost (step.label);
	}
    }
    relaxed_plan.clear();
    return h_ff;
}


static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("FF heuristic", "See also Synergy.");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support(
        "axioms",
        "supported (in the sense that the planner won't complain -- "
        "handling of axioms might be very stupid "
        "and even render the heuristic unsafe)");
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes for tasks without axioms");
    parser.document_property("preferred operators", "yes");

    Heuristic::add_options_to_parser(parser);

    parser.add_option<int>("optimize_relaxed_plan", "If true, computes a relaxed plan where no action is included twice, otherwise just approximates it.", "false");
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new FFHeuristic(opts);
}

static Plugin<Heuristic> _plugin("ff", _parse);
}
