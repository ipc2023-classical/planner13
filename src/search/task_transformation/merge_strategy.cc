#include "merge_strategy.h"

using namespace std;

namespace task_transformation {
MergeStrategy::MergeStrategy(
    const FactoredTransitionSystem &fts)
    : fts(fts) {
}
}
