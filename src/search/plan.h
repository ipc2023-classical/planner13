#ifndef PLAN_H
#define PLAN_H


#include "global_state.h"
#include <vector>


namespace task_representation {
    class FTSTask;
}
class OperatorID;

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

    bool empty() const {
        return !solved;
    }

    void set_plan(const std::vector<GlobalState> & states, const std::vector<int> & labels);
    void set_plan_operators(const std::vector<GlobalState> & states,
                            const std::vector<OperatorID> & operators);

    const std::vector<int> & get_labels ()const {
        return labels;
    }

    const std::vector<GlobalState> & get_traversed_states ()const {
        return states;
    }

};

#endif
