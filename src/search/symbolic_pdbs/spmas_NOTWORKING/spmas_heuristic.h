#ifndef SYMBOLIC_SPMAS_HEURISTIC_H
#define SYMBOLIC_SPMAS_HEURISTIC_H

#include "../../symbolic/sym_controller.h"
#include "../../symbolic/sym_variables.h"
#include "../../heuristic.h"
#include "../../symbolic/sym_solution.h"
#include "../../symbolic/closed_list.h"
#include <memory>

namespace options {
class Options;
}

class GlobalState;

namespace symbolic {
class SymPH;
class SymClosed;

class SPHeuristic {
public:
    std::vector<ADD> heuristicADDs;
    std::vector<int> max_heuristic_value;
    int compute_heuristic(int *state) const;
};

class SPMASHeuristic : public Heuristic, public SymController {
    const std::shared_ptr<task_representation::FTSTask>& task;

    // Search Behavior parameters
    std::vector<SymPH *> phs;

    int generationTime;
    double generationMemory;

    std::vector<SPHeuristic> spHeuristics;
    std::vector<ADD> explicitHeuristics;

    std::vector<BDD> notMutexBDDs;

    SymSolution solution;
    virtual void new_solution(const SymSolution &sol) {
        if (!solution.solved() ||
            sol.getCost() < solution.getCost()) {
            solution = sol;
        }
    }
    void dump_options() const;
    void insert_heuristic(SymClosed *closed);

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);

public:
    SPMASHeuristic(const options::Options &opts, const std::shared_ptr<task_representation::FTSTask>& _task);
    virtual ~SPMASHeuristic() {}
};
}
#endif