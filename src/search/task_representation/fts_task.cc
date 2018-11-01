#include "fts_task.h"

#include "fact.h"
#include "labels.h"
#include "transition_system.h"

#include "../global_state.h"

#include <cassert>

using namespace std;
using namespace task_transformation;


namespace task_representation {
FTSTask::FTSTask(
    vector<unique_ptr<TransitionSystem>> &&transition_systems,
    unique_ptr<Labels> labels)
    : transition_systems(move(transition_systems)),
      labels(move(labels)) {
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

    
}
