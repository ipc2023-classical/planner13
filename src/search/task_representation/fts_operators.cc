#include "fts_operators.h"

using namespace std;

namespace task_representation {
FTSOperator::FTSOperator(
    OperatorID id, LabelID label, int cost, const std::vector<FactPair> &effects)
    : id(id), label(label), cost(cost), effects(effects) {
}
}
