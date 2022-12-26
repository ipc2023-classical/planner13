#include "numeric_dominance_pruning.h"

#include "local_dominance_function.h"
#include "dominance_function.h"
#include "dominance_function_builder.h"

#include "../globals.h"
#include <vector>

using namespace std;

namespace dominance {

template<typename TCost>
NumericDominancePruning<TCost>::NumericDominancePruning(const options::Options &opts)
        : initialized(false),
          dominance_function_builder(opts.get<std::shared_ptr<DominanceFunctionBuilder>>("dominance_analysis")),
          prune_dominated_by_parent(opts.get<bool>("prune_dominated_by_parent")),
          prune_dominated_by_initial_state(opts.get<bool>("prune_dominated_by_initial_state")),
          prune_successors(opts.get<bool>("prune_successors")),
          dump(opts.get<bool>("dump")), exit_after_preprocessing(opts.get<bool>("exit_after_preprocessing"))
          {
}

template<typename TCost>
void NumericDominancePruning<TCost>::dump_options() const {
    cout << "Type pruning: ";
    if (prune_dominated_by_parent) {
        cout << " dominated_by_parent";
    }

    if (prune_dominated_by_parent) {
        cout << " dominated_by_initial_state";
    }
    if (prune_successors) {
        cout << " successors";
    }

    dominance_function_builder->dump_options();
}


template<typename TCost>
bool NumericDominancePruning<TCost>::apply_pruning() const {
    return prune_dominated_by_parent || prune_dominated_by_initial_state || prune_successors;
}

template<typename TCost>
void NumericDominancePruning<TCost>::initialize(const std::shared_ptr<task_representation::FTSTask> &task_) {
    task = task_;
    if (!initialized) {
        dump_options();
        initialized = true;

        if (apply_pruning()) {
            numeric_dominance_relation = dominance_function_builder->compute_dominance_function<TCost>(*task);
        }

        //cout << "Completed preprocessing: " << g_timer() << endl;

        if (exit_after_preprocessing) {
            cout << "Exit after preprocessing." << endl;
            utils::exit_with(utils::ExitCode::UNSOLVED_INCOMPLETE);
        }
    }
}

template<typename TCost>
void NumericDominancePruning<TCost>::prune_operators(const State &state, std::vector<OperatorID> &operators) {
    bool applied_action_selection_pruning = false;

    if (prune_successors && operators.size() > 1) {
        applied_action_selection_pruning = true;
        if (dominance_check.action_selection_pruning(*task, state, operators)) {
            return;
        }
    }

    if (prune_dominated_by_parent || prune_dominated_by_initial_state) {
        dominance_check.prune_dominated_by_parent_or_initial_state(*task, state, operators,
                                                                   applied_action_selection_pruning,
                                                                   prune_dominated_by_parent,
                                                                   prune_dominated_by_initial_state);
    }
}


static shared_ptr<PruningMethod> _parse(options::OptionParser &parser) {
    parser.document_synopsis("Dominance pruning method", "");

    parser.add_option<bool>("dump",
                            "Dumps the relation that has been found",
                            "false");

    parser.add_option<bool>("exit_after_preprocessing",
                            "Exit after preprocessing",
                            "false");

    parser.add_option<bool>("prune_dominated_by_parent",
                            "Prunes a state if it is dominated by its parent",
                            "false");

    parser.add_option<bool>("prune_dominated_by_initial_state",
                            "Prunes a state if it is dominated by the initial state",
                            "false");


    parser.add_option<bool>("prune_successors",
                            "Prunes all siblings if any successor dominates the parent by enough margin",
                            "false");


    parser.add_option<shared_ptr<DominanceFunctionBuilder>>(
            "analysis",
            "Method to perform dominance analysis",
            "num_dominance");



    Options opts = parser.parse();
    //auto cost_type = OperatorCost(opts.get_enum("cost_type"));

    bool task_has_zero_cost = g_main_task->get_min_operator_cost() == 0;

    if (parser.dry_run()) {
        return nullptr;
    } else {
        if (task_has_zero_cost) {
            return make_shared<NumericDominancePruning<IntEpsilon>>(opts);
        } else {
            return make_shared<NumericDominancePruning<int>>(opts);
        }
    }
}


static PluginShared<PruningMethod> _plugin("dominance", _parse);


template
class NumericDominancePruning<int>;

template
class NumericDominancePruning<IntEpsilon>;

}