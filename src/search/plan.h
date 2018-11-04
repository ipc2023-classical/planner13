#ifndef PLAN_H
#define PLAN_H


#include "global_state.h"
#include <vector>


namespace task_representation {
    class FTSTask;
}

class Plan {

    const task_representation::FTSTask *task;
    bool solved;
    std::vector<int> labels;
    std::vector<GlobalState> states;
public:
    Plan (const task_representation::FTSTask *task_) :
    task(task_), solved(false) {

    }
    
    ~Plan() = default;

    void set_plan(std::vector<GlobalState> states, std::vector<int> operators);

    const std::vector<int> & get_labels ()const {
        return labels;
    }

    const std::vector<GlobalState> & get_traversed_states ()const {
        return states;
    }

};

#endif
