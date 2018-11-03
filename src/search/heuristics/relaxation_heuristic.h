#ifndef HEURISTICS_RELAXATION_HEURISTIC_H
#define HEURISTICS_RELAXATION_HEURISTIC_H

#include "../heuristic.h"
#include "../task_representation/fact.h"

#include <vector>

class GlobalState;

namespace relaxation_heuristic {
struct Proposition;
struct UnaryOperator;

struct RelaxedPlanStep {
    int label;
    task_representation::FactPair effect;

RelaxedPlanStep() : label(-1), effect(-1, -1) {
}
    RelaxedPlanStep (int l, task_representation::FactPair effect_) :
    label (l), effect(effect_) {
    }
};

struct UnaryOperator {
    RelaxedPlanStep rp_step;
    std::vector<Proposition *> precondition;
    Proposition *effect;
    int base_cost;

    int unsatisfied_preconditions;
    int cost; // Used for h^max cost or h^add cost;
              // includes operator cost (base_cost)
    UnaryOperator(const std::vector<Proposition *> &pre, Proposition *eff,
                  RelaxedPlanStep step, int base)
    : rp_step(step), precondition(pre), effect(eff), base_cost(base) {}
};

struct Proposition {
    bool is_goal;
    int id;
    std::vector<UnaryOperator *> precondition_of;

    int cost; // Used for h^max cost or h^add cost
    UnaryOperator *reached_by;
    bool marked; // used when computing preferred operators for h^add and h^FF

    Proposition(int id_, bool _is_goal = false) {
        id = id_;
        is_goal = _is_goal;
        cost = -1;
        reached_by = 0;
        marked = false;
    }
};

class RelaxationHeuristic : public Heuristic {
    void simplify();
protected:
    std::vector<UnaryOperator> unary_operators;
    std::vector<std::vector<Proposition>> propositions_per_var;

    std::vector<Proposition *> goal_propositions;

    virtual int compute_heuristic(const GlobalState &state) = 0;
public:
    RelaxationHeuristic(const options::Options &options);
    virtual ~RelaxationHeuristic();
    virtual bool dead_ends_are_reliable() const;

    const Proposition * get_proposition(int var, int val) const {
	return &(propositions_per_var[var][val]);
    }
};
}

#endif

