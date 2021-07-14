#ifndef SYMBOLIC_SYM_SOLUTION_H
#define SYMBOLIC_SYM_SOLUTION_H

#include "sym_variables.h"
#include "../operator_id.h"
#include <vector>

namespace symbolic {
class UnidirectionalSearch;

class SymSolution {
    UnidirectionalSearch *exp_fw, *exp_bw;
    int g, h;
    BDD cut;
    const std::shared_ptr<task_representation::FTSTask> &task;
public:
    SymSolution(const std::shared_ptr<task_representation::FTSTask> &_task) : g(-1), h(-1), task(_task) {} //No solution yet
    SymSolution(const SymSolution &other) = default;

    SymSolution(UnidirectionalSearch *e_fw, UnidirectionalSearch *e_bw, int g_val, int h_val, BDD S, const std::shared_ptr<task_representation::FTSTask> &_task)
        : exp_fw(e_fw), exp_bw(e_bw), g(g_val), h(h_val), cut(S), task(_task) { }

    void getPlan(std::vector<PlanState>& states, std::vector <OperatorID> &path) const;

    ADD getADD() const;

    inline bool solved() const {
        return g + h >= 0;
    }

    inline int getCost() const {
        return g + h;
    }

    SymSolution& operator= (const SymSolution &other) {
        if (this == &other)
            return *this;

        this->g = other.g;
        this->h = other.h;
        this->cut = other.cut;
        this->exp_bw = other.exp_bw;
        this->exp_fw = other.exp_fw;
        return *this;
    }
};
}
#endif
