#ifndef TASK_REPRESENTATION_FTS_TASK_H
#define TASK_REPRESENTATION_FTS_TASK_H

#include <memory>
#include <vector>


#include "../operator_cost.h"

class GlobalState;

namespace int_packer {
    class IntPacker;
}

namespace task_transformation {
class FactoredTransitionSystem;
}

namespace task_representation {
struct FactPair;
class Label;
class Labels;
class SASTask;
class SearchTask;
class TransitionSystem;


class FTSTask {
    std::vector<std::unique_ptr<TransitionSystem>> transition_systems;
    std::unique_ptr<Labels> labels;

    mutable std::vector<std::vector<int>> label_preconditions;
    mutable std::shared_ptr<SearchTask> search_task;
public:
    FTSTask(
        const std::vector<std::unique_ptr<TransitionSystem>> &transition_systems_,
        const std::unique_ptr<Labels> & labels_);

    FTSTask(
        std::vector<std::unique_ptr<TransitionSystem>> &&transition_systems,
        std::unique_ptr<Labels> labels);


    FTSTask(const FTSTask & other, OperatorCost cost_type);

    ~FTSTask();
    FTSTask(FTSTask &&other) = delete;
    FTSTask(const FTSTask &other) = delete;
    FTSTask &operator=(const FTSTask &) = delete;

    int get_num_labels() const;
    int get_label_cost(int label) const;
    int get_min_operator_cost() const;
    bool is_goal_state (const GlobalState &state) const;
    const std::vector<int> &get_label_preconditions(int label) const;
    std::vector<int> get_goal_variables() const;

    int get_size() const {
        return int(transition_systems.size());
    }

    const TransitionSystem &get_ts(int index) const {
        return *transition_systems[index];
    }

    const std::vector<std::unique_ptr<TransitionSystem>>& get_transition_systems() const {
        return transition_systems;
    }

    const Labels &get_labels() const {
        return *labels;
    }


    std::vector<int> get_initial_state() const ;
    bool trivially_solved() const;

    std::shared_ptr<SearchTask> get_search_task(bool print_time = false) const;

    void dump() const;
    friend std::ostream &operator<<(std::ostream &os, const FTSTask &task);    
};
}


#endif
