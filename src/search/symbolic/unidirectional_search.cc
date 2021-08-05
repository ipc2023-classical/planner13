#include "unidirectional_search.h"

#include "sym_solution.h"
#include "closed_list.h"

using namespace std;

namespace symbolic {
    OppositeFrontierFixed::OppositeFrontierFixed(BDD bdd, const SymStateSpaceManager & mgr, const std::shared_ptr<task_representation::FTSTask> &_task)
        : OppositeFrontier(_task), goal (bdd), hNotGoal(mgr.getAbsoluteMinTransitionCost()) {
    }

    SymSolution OppositeFrontierFixed::checkCut(UnidirectionalSearch * search, const BDD &states, int g, bool fw) const {
	BDD cut = states * goal;
	if (cut.IsZero()) {
	    return SymSolution(task); //No solution yet :(
	}

	if (fw) //Solution reconstruction will fail
	    return SymSolution(search, nullptr,  g, 0, cut, task);
	else
	    return SymSolution(nullptr, search, 0, g, cut, task);

    }
    UnidirectionalSearch::UnidirectionalSearch(SymController * eng, const SymParamsSearch &params, const std::shared_ptr<task_representation::FTSTask>& _task) :
	SymSearch(eng, params), fw(true), closed(make_shared<ClosedList>(_task)) { }


    void UnidirectionalSearch::statistics() const {
	cout << "Exp " << (fw ? "fw" : "bw") << " time: " << stats.step_time << "s (img:" <<
	    stats.image_time << "s, heur: " << stats.time_heuristic_evaluation <<
	    "s) in " << stats.num_steps_succeeded << " steps ";
    }
}
