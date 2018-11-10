#include "fts_task.h"

#include "fact.h"
#include "labels.h"
#include "transition_system.h"

#include "../global_state.h"
#include "search_task.h"
#include <cassert>

using namespace std;
using namespace task_transformation;


namespace task_representation {
    FTSTask::FTSTask(
        vector<unique_ptr<TransitionSystem>> &&transition_systems_,
        unique_ptr<Labels> labels)
        : transition_systems(move(transition_systems_)),
          labels(move(labels)) {

        cout << "Constructing FTS Task: " << transition_systems.size() << endl;
#ifndef NDEBUG
        for (int label = 0; label < this->labels->get_size(); ++label) {
            assert(this->labels->is_current_label(label));
        }

        for (const auto &ts : this->transition_systems) {
            assert(ts);
            assert(ts->get_size() > 1);
            // TODO: re-enable this assertions once we remove dead labels
//            for (const auto &tr: *ts) { //assert that there are no dead labels
//                assert(!tr.transitions.empty());
//            }
        }
#endif
    }

FTSTask::~FTSTask() {
    for (auto &transition_system : transition_systems) {
        transition_system = nullptr;
    }
    labels = nullptr;
}

int FTSTask::get_num_labels() const {
    return labels->get_size();
}

int FTSTask::get_label_cost(int label) const {
    return labels->get_label_cost(label);
}

int FTSTask::get_min_operator_cost() const {
    if (!labels->get_size()) {
        return 0;
    }
    assert(labels->is_current_label(0));
    int minimum_cost = labels->get_label_cost(0);
    for (int label = 0; label < labels->get_size(); ++label) {
        assert(labels->is_current_label(label));
        minimum_cost = std::min(minimum_cost, labels->get_label_cost(label));
    }
    return minimum_cost;
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
    
    assert(label >= 0 && label < static_cast<int>(label_preconditions.size()));
    return label_preconditions[label];
}

vector<int> FTSTask::get_goal_variables() const {
    vector<int> goal_variables;
    for (size_t var = 0; var < transition_systems.size(); ++var) {
        const TransitionSystem &ts = *transition_systems[var];
        if (ts.is_goal_relevant()) {
            goal_variables.push_back(var);
        }
    }
    return goal_variables;
}



shared_ptr<SearchTask> FTSTask::get_search_task() const {
    if(!search_task){
	search_task = make_shared<SearchTask> (*this);
    }
    return search_task;
}


void FTSTask::dump() const {
    for (const auto & ts: transition_systems) {
        ts->dump_labels_and_transitions();
    }
}
    
    bool FTSTask::trivially_solved() const {
        return transition_systems.size() == 0 ||
            std::all_of (transition_systems.begin(),
                         transition_systems.end(),
                         [] (const unique_ptr<TransitionSystem> & ts) {
                             return ts->is_goal_state(ts->get_init_state());
                         }
                ); 
    }

}
