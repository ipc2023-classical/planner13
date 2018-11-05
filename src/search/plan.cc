#include "plan.h"

#include <iostream>
#include <fstream>
#include <cassert>

#include "operator_id.h"
#include "task_representation/fts_task.h"
#include "task_representation/search_task.h"

void Plan::set_plan(const std::vector<GlobalState> & states_, const std::vector<int>& labels_) {
    states = states_;
    labels = labels_;
    solved=true;
    assert(states.size() == labels.size() + 1);
}

void Plan::set_plan_operators(const std::vector<GlobalState> & states_,
                              const std::vector<OperatorID> & operators) {
    states = states_;
    labels.reserve(operators.size());
    for(auto op : operators) {
        labels.push_back(task->get_search_task()->get_label (op));
    }
    solved=true;
    assert(states.size() == labels.size() + 1);
}

