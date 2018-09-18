#include "fts_successor_generator.h"


using namespace std;

namespace task_representation {
FTSSuccessorGenerator::FTSSuccessorGenerator(const FTSTask &task_proxy) {
}
FTSSuccessorGenerator::~FTSSuccessorGenerator() = default;

void FTSSuccessorGenerator::generate_applicable_ops(
    const State &state, vector<OperatorID> &applicable_ops) const {
    root->generate_applicable_ops(state, applicable_ops);
}

void FTSSuccessorGenerator::generate_applicable_ops(
    const GlobalState &state, vector<OperatorID> &applicable_ops) const {
    root->generate_applicable_ops(state, applicable_ops);
}
}
