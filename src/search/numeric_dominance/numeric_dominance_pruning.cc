#include "numeric_dominance_pruning.h"

#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"

#include "numeric_simulation_relation.h"
#include "numeric_dominance_relation.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../task_representation/state.h"
#include "../utils/timer.h"
#include "../search_progress.h"

#include <cassert>
#include <vector>
#include <limits>

using namespace std;

namespace numeric_dominance {

template<typename T>
NumericDominancePruning<T>::NumericDominancePruning(const options::Options &opts)
        : initialized(false),
          tau_labels(make_shared<TauLabelManager<T>>(opts, false)),
          prune_dominated_by_parent(opts.get<bool>("prune_dominated_by_parent")),
          prune_dominated_by_initial_state(opts.get<bool>("prune_dominated_by_initial_state")),
          prune_successors(opts.get<bool>("prune_successors")),
          truncate_value(opts.get<int>("truncate_value")),
          max_simulation_time(opts.get<int>("max_simulation_time")),
          min_simulation_time(opts.get<int>("min_simulation_time")),
          max_total_time(opts.get<int>("max_total_time")),
          max_lts_size_to_compute_simulation(opts.get<int>("max_lts_size_to_compute_simulation")),
          num_labels_to_use_dominates_in(opts.get<int>("num_labels_to_use_dominates_in")),
          dump(opts.get<bool>("dump")), exit_after_preprocessing(opts.get<bool>("exit_after_preprocessing")),
          all_desactivated(false), activation_checked(false),
          states_inserted(0), states_checked(0), states_pruned(0), deadends_pruned(0) {
}

template<typename T>
void NumericDominancePruning<T>::dump_options() const {
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

    tau_labels->print_config();

    cout << "truncate_value: " << truncate_value << endl <<
         "num_labels_to_use_dominates_in: " << num_labels_to_use_dominates_in << endl <<
         "max_lts_size_to_compute_simulation: " << max_lts_size_to_compute_simulation << endl <<
         "max_simulation_time: " << max_simulation_time << endl <<
         "min_simulation_time: " << min_simulation_time << endl <<
         "max_total_time: " << max_total_time << endl;
}


template<typename T>
bool NumericDominancePruning<T>::apply_pruning() const {
    return prune_dominated_by_parent || prune_dominated_by_initial_state || prune_successors;
}

template<typename T>
void NumericDominancePruning<T>::initialize(const std::shared_ptr<task_representation::FTSTask> &task_) {
    task = task_;
    if (!initialized) {
        dump_options();
        initialized = true;

        if (apply_pruning()) {
            numeric_dominance_relation = make_unique<NumericDominanceRelation<T>>(task,
                                                                                  truncate_value, max_simulation_time,
                                                                                  min_simulation_time, max_total_time,
                                                                                  max_lts_size_to_compute_simulation,
                                                                                  num_labels_to_use_dominates_in,
                                                                                  tau_labels);

            vector<TransitionSystem> tss;
            tss.reserve(task->get_size());
            for (int i = 0; i < task->get_size(); i++) {
                tss.push_back(task->get_ts(i));
            }

            numeric_dominance_relation->init();
            numeric_dominance_relation->compute_ld_simulation(tss, task->get_labels(), dump);
        }

        //cout << "Completed preprocessing: " << g_timer() << endl;

        if (exit_after_preprocessing) {
            cout << "Exit after preprocessing." << endl;
            utils::exit_with(utils::ExitCode::UNSOLVED_INCOMPLETE);
        }
    }
}

template<typename T>
void NumericDominancePruning<T>::prune_operators(const State &state, std::vector<OperatorID> &operators) {
    bool applied_action_selection_pruning = false;

    if (prune_successors && operators.size() > 1) {
        applied_action_selection_pruning = true;
        if (numeric_dominance_relation->action_selection_pruning(state, operators)) {
            return;
        }
    }

    if (prune_dominated_by_parent || prune_dominated_by_initial_state) {
        numeric_dominance_relation->prune_dominated_by_parent_or_initial_state(state, operators,
                                                                               applied_action_selection_pruning,
                                                                               prune_dominated_by_parent,
                                                                               prune_dominated_by_initial_state);
    }
}


static shared_ptr<PruningMethod> _parse(options::OptionParser &parser) {
    parser.document_synopsis("Simulation heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional_effects", "supported (but see note)");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.document_note(
            "Note",
            "Conditional effects are supported directly. Note, however, that "
            "for tasks that are not factored (in the sense of the JACM 2014 "
            "merge-and-shrink paper), the atomic abstractions on which "
            "merge-and-shrink heuristics are based are nondeterministic, "
            "which can lead to poor heuristics even when no shrinking is "
            "performed.");


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

    parser.add_option<int>("truncate_value",
                           "Assume -infinity if below minus this value",
                           "1000");

    parser.add_option<int>("max_simulation_time",
                           "Maximum number of seconds spent in computing a single update of a simulation", "1800000");

    parser.add_option<int>("min_simulation_time",
                           "Minimum number of seconds spent in computing a single update of a simulation",
                           "100000"); // By default we do not have any limit

    parser.add_option<int>("max_total_time",
                           "Maximum number of seconds spent in computing all updates of a simulation", "1800000");

    parser.add_option<int>("max_lts_size_to_compute_simulation",
                           "Avoid computing simulation on ltss that have more states than this number",
                           "1000000");

    parser.add_option<int>("num_labels_to_use_dominates_in",
                           "Use dominates_in for instances that have less than this amount of labels",
                           "0");

    parser.add_option<bool>("prune_successors",
                            "Prunes all siblings if any successor dominates the parent by enough margin",
                            "false");

    TauLabelManager<int>::add_options_to_parser(parser);

    Options opts = parser.parse();
    //auto cost_type = OperatorCost(opts.get_enum("cost_type"));

    // TODO: Make sure this is valid
    bool task_has_zero_cost = g_main_task->get_min_operator_cost() ==
                              0;//cost_type == OperatorCost::ZERO || (cost_type == OperatorCost::NORMAL && g_min_action_cost == 0);

    if (parser.dry_run()) {
        return 0;
    } else {
        if (task_has_zero_cost) {
            return make_shared<NumericDominancePruning<IntEpsilon>>(opts);
        } else {
            return make_shared<NumericDominancePruning<int>>(opts);
        }
    }
}


static PluginShared<PruningMethod> _plugin("num_dominance", _parse);


template
class NumericDominancePruning<int>;

template
class NumericDominancePruning<IntEpsilon>;

}