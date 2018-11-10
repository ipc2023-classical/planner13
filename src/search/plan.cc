#include "plan.h"

#include <iostream>
#include <fstream>
#include <cassert>

#include "operator_id.h"
#include "task_representation/fts_task.h"
#include "task_representation/search_task.h"

using namespace std;


Plan::Plan (const task_representation::FTSTask *task_) :
    task(task_), solved(false) {
    if (task->trivially_solved()) {
        solved = true;
        
    }
    }

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

//    cout << "Plan found" << endl;
//    states[0].dump_fdr();
//    for(size_t i = 0; i < operators.size(); ++i) {
//        cout << endl << " Operator " << operators[i].get_index() << " ";
//        task->get_search_task()->dump_op(operators[i]);
//        cout << endl;
//        states[i+1].dump_fdr();
//    }
}

