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
class Labels;
class SASTask;
class SearchTask;
class TransitionSystem;


class FTSTask {
    std::vector<std::unique_ptr<TransitionSystem>> transition_systems;
    std::unique_ptr<Labels> labels;

    mutable std::vector<std::vector<int>> label_preconditions;
public:
    FTSTask(
        std::vector<std::unique_ptr<TransitionSystem>> &&transition_systems,
        std::unique_ptr<Labels> labels);
    ~FTSTask();
    FTSTask(FTSTask &&other) = delete;
    FTSTask(const FTSTask &other) = delete;
    FTSTask &operator=(const FTSTask &) = delete;

    const TransitionSystem &get_ts(int index) const {
        return *transition_systems[index];
    }


    // Used by LabelReduction and MergeScoringFunctionDFP
    const Labels &get_labels() const {
        return *labels;
    }

    int get_size() const {
        return transition_systems.size();
    }

    int get_label_cost(int label) const;

    int get_min_operator_cost() const;

    int get_num_labels() const;

    bool is_goal_state (const GlobalState & state) const;

    const std::vector<int> & get_label_preconditions(int label) const;
};
}


#endif
