#ifndef TASK_REPRESENTATION_FTS_TASK_H
#define TASK_REPRESENTATION_FTS_TASK_H

#include <memory>
#include <vector>

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
        std::vector<std::unique_ptr<TransitionSystem>> &&transition_systems,
        std::unique_ptr<Labels> labels);
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
        return transition_systems.size();
    }

    const TransitionSystem &get_ts(int index) const {
        return *transition_systems[index];
    }

    const Labels &get_labels() const {
        return *labels;
    }

    std::shared_ptr<SearchTask> get_search_task() const;

    void dump() const; 
};
}


#endif
