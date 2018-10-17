#ifndef FTS_REPRESENTATION_FTS_OPERATORS_H
#define FTS_REPRESENTATION_FTS_OPERATORS_H

#include "fact.h"
#include "types.h"

#include "../operator_id.h"

#include <vector>

namespace task_representation {
class FTSTask;

class FTSOperator {
    OperatorID id; // Unique identifier of the operator in the planning task
    LabelID label; // Label that corresponds to this operator
    int cost;
    std::vector<FactPair> effects;
public:
    FTSOperator(
        OperatorID id, LabelID label, int cost, const std::vector<FactPair> &effects);

    OperatorID get_id() const {
        return id;
    }

    LabelID get_label() const {
        return label;
    }

    int get_cost() const {
        return cost;
    }

    const std::vector<FactPair> &get_effects() const {
        return effects;
    }
};

//class FTSOperators {
//    std::vector<FTSOperator> operators_by_id;
//    std::vector<std::vector<int> > operators_by_label;
//public:
//    explicit FTSOperators(const FTSTask &fts_task);

//    const FTSOperator &get_operator(OperatorID id_op) const {
//        return operators_by_id[id_op.get_index()];
//    }

//};
}


#endif
