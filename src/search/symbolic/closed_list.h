#ifndef SYMBOLIC_CLOSED_LIST_H
#define SYMBOLIC_CLOSED_LIST_H

#include "sym_variables.h"
#include "unidirectional_search.h"
#include "../task_representation/fts_operators.h"

#include <vector>
#include <set>
#include <map>

namespace symbolic {

class SymStateSpaceManager;
class SymSolution;
class UnidirectionalSearch;
class SymSearch;

class ClosedList : public OppositeFrontier {
private:
    UnidirectionalSearch * my_search;
    SymStateSpaceManager *mgr;  //Symbolic manager to perform bdd operations
    const std::shared_ptr<task_representation::FTSTask> &task; // FTSTask

    std::map<int, BDD> closed;   // Mapping from cost to set of states

    // Auxiliar BDDs for the number of 0-cost action steps
    // ALERT: The information here might be wrong
    // It is just used to extract path more quickly, but the information
    // here is an (admissible) estimation and this should be taken into account
    std::map<int, std::vector<BDD>> zeroCostClosed;
    BDD closedTotal;             // All closed states.

    int hNotClosed, fNotClosed; // Bounds on h and g for those states not in closed
    std::map<int, BDD> closedUpTo;  // Disjunction of BDDs in closed  (auxiliar useful to take the maximum between several BDDs)
    std::set<int> h_values; //Set of h_values of the heuristic

    void newHValue(int h_value); 

public:
    virtual ~ClosedList() = default;
    explicit ClosedList(const std::shared_ptr<task_representation::FTSTask> &_task);
    void init(SymStateSpaceManager *manager, UnidirectionalSearch * search);
    void init(SymStateSpaceManager *manager, UnidirectionalSearch * search, const ClosedList &other);

    void insert(int h, const BDD &S);
    void setHNotClosed(int h);
    void setFNotClosed(int f);
    
    const std::set<int> &getHValues();

    //Check if any of the states is closed.
    //In case positive, return a solution pair <f_value, S>
    virtual SymSolution checkCut(UnidirectionalSearch * search, const BDD &states, int g, bool fw) const override;

    void extract_path(const BDD &cut, int h, bool fw,
		      std::vector <OperatorID> &path) const;

    inline BDD getClosed() const {
        return closedTotal;
    }

    virtual BDD notClosed() const override {
        return !closedTotal;
    }

    inline std::map<int, BDD> getClosedList() const {
        return closed;
    }

    inline int getHNotClosed() const override {
        return hNotClosed;
    }

    inline int getFNotClosed() const {
        return fNotClosed;
    }

    ADD getHeuristic(int previousMaxH = -1) const;

    void getHeuristic(std::vector<ADD> &heuristics,
                      std::vector <int> &maxHeuristicValues) const;

    void statistics() const;


    double average_hvalue() const;

    virtual bool exhausted () const override {
	    return fNotClosed == std::numeric_limits<int>::max();
    }

    BDD setSuccWithEffects(const BDD &cut, const task_representation::FTSOperator &op,
                           int fw, const TransitionRelation &tr) const;

    void assertOpIdExists(const TransitionRelation &tr, const std::vector<task_representation::FTSOperator> &fts_ops) const;
};
}

#endif // SYM_CLOSED
