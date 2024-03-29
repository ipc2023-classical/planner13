#ifndef SYMBOLIC_GAMER_PDBS_HEURISTIC_H
#define SYMBOLIC_GAMER_PDBS_HEURISTIC_H

#include "../symbolic/sym_controller.h"
#include "../heuristic.h"
#include "../symbolic/sym_solution.h"


namespace symbolic {

class SymStateSpaceManager;
class OriginalStateSpace;
class UniformCostSearch;

class GamerPDBsHeuristic;

class PDBSearch {
    GamerPDBsHeuristic * spdbheuristic;

    std::set<int> pattern;

    std::shared_ptr <SymStateSpaceManager> state_space;
    std::unique_ptr <UniformCostSearch> uc_search;
    double average_hval;
public:
    const std::shared_ptr<task_representation::FTSTask> &task;

    PDBSearch(GamerPDBsHeuristic *spdbheuristic_, std::shared_ptr<SymStateSpaceManager> originalStateSpace,
              const std::shared_ptr<task_representation::FTSTask> &_task);
    PDBSearch(const std::set<int> &pattern_, GamerPDBsHeuristic *spdbheuristic_,
              const std::shared_ptr<OriginalStateSpace> &originalStateSpace,
              const std::shared_ptr<task_representation::FTSTask> &_task);


    void search(const SymParamsSearch & searchParams, int generationTime = 0, double generationMemory = 0);

    ADD getHeuristic() const;
    double average_value();

    const std::set<int>  & get_pattern() const {
	return pattern;
    }

    std::vector<int> candidate_vars() const;

    UniformCostSearch * get_search() {
	return uc_search.get();
    }
};

std::ostream & operator<<(std::ostream &os, const PDBSearch & pdb);

class GamerPDBsHeuristic : public Heuristic, public SymController {
    const int generationTime;
    const double generationMemory;
    const bool useSuperPDB;
    const bool perimeter;

    int max_perimeter_heuristic;
    std::unique_ptr<ADD> perimeter_heuristic;
    std::unique_ptr<ADD> heuristic;
    std::vector<BDD> notMutexBDDs;

    void dump_options() const;

    bool influences(int var, const std::set<int> & pattern);

    void initialize();
protected:

    virtual int compute_heuristic(const GlobalState &state) override;

public:
    GamerPDBsHeuristic(const options::Options &opts);
    virtual ~GamerPDBsHeuristic() = default;
};

}

#endif
