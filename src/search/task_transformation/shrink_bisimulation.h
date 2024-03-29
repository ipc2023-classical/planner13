#ifndef TASK_TRANSFORMATION_SHRINK_BISIMULATION_H
#define TASK_TRANSFORMATION_SHRINK_BISIMULATION_H

#include "shrink_strategy.h"

namespace options {
class Options;
}
namespace task_representation {
    class TransitionSystem;
}

namespace task_transformation {
struct Signature;

using namespace task_representation;

class ShrinkBisimulation : public ShrinkStrategy {
    enum AtLimit {
        RETURN,
        USE_UP
    };

    const bool greedy;
    const AtLimit at_limit;
    const int max_size_after_shrink;
    const int min_size_to_shrink;

    
    void compute_abstraction(
        const TransitionSystem &ts,
        const Distances &distances,
        int target_size,
        StateEquivalenceRelation &equivalence_relation) const;

    int initialize_groups(
        const TransitionSystem &ts,
        const Distances &distances,
        std::vector<int> &state_to_group) const;

    void compute_signatures(
        const TransitionSystem &ts,
        const Distances &distances,
        std::vector<Signature> &signatures,
        const std::vector<int> &state_to_group) const;
protected:
    virtual void dump_strategy_specific_options() const override;
    virtual std::string name() const override;
public:
    explicit ShrinkBisimulation(const options::Options &opts);
    virtual ~ShrinkBisimulation() override = default;

    virtual StateEquivalenceRelation compute_equivalence_relation(
        const FactoredTransitionSystem &fts,
        int index, int target_size) const override;

    
    virtual bool apply_shrinking_transformation(FactoredTransitionSystem &fts,
                                                Verbosity verbosity) const override;
    
    virtual bool apply_shrinking_transformation(FactoredTransitionSystem &fts,
                                                Verbosity verbosity, int & index) const override;

    virtual bool requires_init_distances() const override {
        return false;
    }

    virtual bool requires_goal_distances() const override {
        return true;
    }

    static std::shared_ptr<ShrinkStrategy> create_default_perfect();
};
}

#endif
