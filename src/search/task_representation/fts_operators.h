#ifndef FTS_REPRESENTATION_FTS_OPERATORS_H
#define FTS_REPRESENTATION_FTS_OPERATORS_H

#include <vector>

#include "../operator_id.h"
#include "fact.h"

using namespace std;

namespace task_representation {
class FTSOperator {
    OperatorID id; // Unique identifier of the operator in the planning task
    int label; // Label that corresponds to this operator
    int cost;

    std::vector<FactPair> effects;
public:
    OperatorID get_id () const {
        return id;
    }

    /* std::vector<Fact> targets; */

    const std::vector<FactPair> & get_effects() const {
        return effects;
    }

    int get_cost() const {
        return cost;
    }
};


class FTSOperators {
    std::vector<FTSOperator> operators_by_id;
    std::vector<std::vector<int> > operators_by_label;

public:
    const FTSOperator & get_operator (OperatorID id_op) const {
        return operators_by_id[id_op.get_index()];
    }

};
}


#endif
