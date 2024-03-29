#ifndef HEURISTICS_FF_HEURISTIC_H
#define HEURISTICS_FF_HEURISTIC_H

#include "additive_heuristic.h"

#include <vector>
#include <memory>

namespace ff_heuristic {
using Proposition = relaxation_heuristic::Proposition;
using UnaryOperator = relaxation_heuristic::UnaryOperator;
using RelaxedPlanStep = relaxation_heuristic::RelaxedPlanStep;

/*
  TODO: In a better world, this should not derive from
        AdditiveHeuristic. Rather, the common parts should be
        implemented in a common base class. That refactoring could be
        made at the same time at which we also unify this with the
        other relaxation heuristics and the additional FF heuristic
        implementation in the landmark code.
*/
class FFHeuristic : public additive_heuristic::AdditiveHeuristic {
    std::vector<RelaxedPlanStep> relaxed_plan;

    const bool optimize_relaxed_plan; 
   
    void relaxed_plan_extraction(Proposition *goal);
protected:
    virtual int compute_heuristic(const GlobalState &global_state);
public:
    FFHeuristic(const options::Options &options);
    ~FFHeuristic();
};
}

#endif
