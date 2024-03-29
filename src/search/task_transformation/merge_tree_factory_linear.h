#ifndef MERGE_AND_SHRINK_MERGE_TREE_FACTORY_LINEAR_H
#define MERGE_AND_SHRINK_MERGE_TREE_FACTORY_LINEAR_H

#include "merge_tree_factory.h"

#include "variable_order_finder.h"

namespace task_transformation {
class MergeTreeFactoryLinear : public MergeTreeFactory {
    VariableOrderType variable_order_type;
protected:
    virtual std::string name() const override;
    virtual void dump_tree_specific_options() const override;
public:
    explicit MergeTreeFactoryLinear(const options::Options &options);
    virtual ~MergeTreeFactoryLinear() override = default;
    virtual std::unique_ptr<MergeTree> compute_merge_tree(
        const FTSTask &fts_task) override;
    virtual std::unique_ptr<MergeTree> compute_merge_tree(
        const FTSTask &fts_task,
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices_subset) override;

    virtual bool requires_init_distances() const override {
        return false;
    }

    virtual bool requires_goal_distances() const override {
        return false;
    }

    static void add_options_to_parser(options::OptionParser &parser);
};
}

#endif
