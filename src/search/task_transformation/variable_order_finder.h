#ifndef TASK_TRANSFORMATION_VARIABLE_ORDER_FINDER_H
#define TASK_TRANSFORMATION_VARIABLE_ORDER_FINDER_H

#include <memory>
#include <vector>

namespace task_representation {
class FTSTask;
}

using namespace task_representation;

namespace task_transformation {
enum VariableOrderType {
    CG_GOAL_LEVEL,
    CG_GOAL_RANDOM,
    GOAL_CG_LEVEL,
    RANDOM,
    LEVEL,
    REVERSE_LEVEL
};

extern void dump_variable_order_type(VariableOrderType variable_order_type);

/*
  NOTE: VariableOrderFinder keeps a reference to the task proxy passed to the
  constructor. Therefore, users of the class must ensure that the task lives at
  least as long as the variable order finder.
*/
class VariableOrderFinder {
    const VariableOrderType variable_order_type;
    std::vector<int> selected_vars;
    std::vector<int> remaining_vars;
    std::vector<bool> is_goal_variable;
//    std::vector<bool> is_causal_predecessor;

    void select_next(int position, int var_no);
public:
    VariableOrderFinder(const FTSTask &fts_task,
                        VariableOrderType variable_order_type);
    ~VariableOrderFinder() = default;
    bool done() const;
    int next();
};
}

#endif
