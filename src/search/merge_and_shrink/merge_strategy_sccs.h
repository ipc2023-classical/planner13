#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_SCCS_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_SCCS_H

#include "merge_strategy.h"

#include <memory>
#include <vector>

namespace task_representation {
class FTSTask;
}

namespace merge_and_shrink {
class MergeSelector;
class MergeTreeFactory;
class MergeTree;
class MergeStrategySCCs : public MergeStrategy {
    const task_representation::FTSTask &fts_task;
    std::shared_ptr<MergeTreeFactory> merge_tree_factory;
    std::shared_ptr<MergeSelector> merge_selector;
    std::vector<std::vector<int>> non_singleton_cg_sccs;
    std::vector<int> indices_of_merged_sccs;

    // Active "merge strategies" while merging a set of indices
    std::unique_ptr<MergeTree> current_merge_tree;
    std::vector<int> current_ts_indices;
public:
    MergeStrategySCCs(
        const FactoredTransitionSystem &fts,
        const task_representation::FTSTask &fts_task,
        const std::shared_ptr<MergeTreeFactory> &merge_tree_factory,
        const std::shared_ptr<MergeSelector> &merge_selector,
        std::vector<std::vector<int>> non_singleton_cg_sccs,
        std::vector<int> indices_of_merged_sccs);
    virtual ~MergeStrategySCCs() override;
    virtual std::pair<int, int> get_next() override;
};
}

#endif
