#ifndef FTS_REPRESENTATION_FTS_TASK_H
#define FTS_REPRESENTATION_FTS_TASK_H

#include "types.h"

#include "fts_operators.h"

#include <memory>
#include <vector>

class GlobalState;

namespace int_packer {
    class IntPacker;
}

namespace task_representation {
class Distances;
class FactoredTransitionSystem;
struct FactPair;
class FTSOperators;
class Labels;
class SASTask;
class SearchTask;
class State;
class TransitionSystem;


class FTSTask {
    std::unique_ptr<Labels> labels;
    std::vector<std::unique_ptr<TransitionSystem>> transition_systems;

public:
    FTSTask(const SASTask & sas_task); //Creates the fts task from the SAS+ representation
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

    std::string get_fact_name(const FactPair & fact) const;

    bool is_goal_state (const GlobalState & state) const;

    const std::vector<int> & get_label_preconditions(int label) const;
};
}


#endif
