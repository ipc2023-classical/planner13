#include "fts_task.h"

#include "factored_transition_system.h"
#include "fts_factory.h"
#include "label_equivalence_relation.h"
#include "labels.h"
#include "sas_operator.h"
#include "sas_task.h"
#include "state.h"
#include "transition_system.h"

#include "../utils/collections.h"
#include "../utils/memory.h"
#include "../utils/system.h"
#include "../algorithms/int_packer.h"

#include <cassert>

using namespace std;


namespace task_representation {
FTSTask::FTSTask(const SASTask &sas_task) {
    // TODO: turn into option
    Verbosity verbosity(Verbosity::NORMAL);
    FactoredTransitionSystem fts =
        create_factored_transition_system(
            sas_task,
            verbosity);

    // Possibly add further M&S-like transformations

    int num_factors = fts.get_num_active_entries();
    transition_systems.reserve(num_factors);
    for (int ts_index : fts) {
        if (fts.is_active(ts_index)) {
            transition_systems.push_back(move(fts.extract_transition_system(ts_index)));
        }
    }
    labels = move(fts.extract_labels());
}

FTSTask::~FTSTask() {
    labels = nullptr;
    for (auto &transition_system : transition_systems) {
        transition_system = nullptr;
    }
}

int FTSTask::get_label_cost(int label) const {
    return labels->get_label_cost(LabelID(label));
}

int FTSTask::get_num_labels() const {
    return labels->get_size();
}

int FTSTask::get_min_operator_cost() const {
    return labels->get_min_operator_cost();
}

std::string FTSTask::get_fact_name(const FactPair & fp) const {
    return "fact" + std::to_string(fp.var) + "-" + std::to_string(fp.value);
}

bool FTSTask::is_goal_state(const GlobalState & state) const {
    // TODO: this is duplicate with SearchTask. Use SearchTask everywhere?
    for (size_t i = 0; i < transition_systems.size(); ++i) {
        const TransitionSystem &ts = *transition_systems[i];
        if (!ts.is_goal_state(state[i])) {
            return false;
        }
    }
    return true;
}

const std::vector<int> & FTSTask::get_label_preconditions(int label) const {
    if (label_preconditions.empty()) {
        int num_labels = get_num_labels();
        label_preconditions.resize(num_labels);
        for (int label_no = 0; label_no < num_labels; ++label_no) {
            for (size_t index = 0; index < transition_systems.size(); ++index) {
                if (transition_systems[index]->has_precondition_on(LabelID(label_no))) {
                    label_preconditions[label_no].push_back(index);
                }
            }
        }
    }
    
    assert(label >= 0 && label < label_preconditions.size()); 
    return label_preconditions[label];
}

    
}
