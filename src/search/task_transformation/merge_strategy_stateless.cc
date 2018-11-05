#include "merge_strategy_stateless.h"

#include "merge_selector.h"

using namespace std;

namespace task_transformation {
MergeStrategyStateless::MergeStrategyStateless(
    const FactoredTransitionSystem &fts,
    const shared_ptr<MergeSelector> &merge_selector)
    : MergeStrategy(fts),
      merge_selector(merge_selector) {
}

pair<int, int> MergeStrategyStateless::get_next(
    const vector<int> &allowed_indices) {
    return merge_selector->select_merge(fts, allowed_indices);
}
}
