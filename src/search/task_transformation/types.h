#ifndef TASK_TRANSFORMATION_TYPES_H
#define TASK_TRANSFORMATION_TYPES_H

#include <forward_list>
#include <vector>
#include <memory>

namespace task_transformation {
// Positive infinity. The name "INFINITY" is taken by an ISO C99 macro.
extern const int INF;
extern const int MINUSINF;
extern const int PRUNED_STATE;

/*
  An equivalence class is a set of abstract states that shall be
  mapped (shrunk) to the same abstract state.

  An equivalence relation is a partitioning of states into
  equivalence classes. It may omit certain states entirely; these
  will be dropped completely and receive an h value of infinity.
*/
using StateEquivalenceClass = std::forward_list<int>;
using StateEquivalenceRelation = std::vector<StateEquivalenceClass>;

class StateMapping;
class LabelMap;

struct Mapping {
    std::shared_ptr<StateMapping> state_mapping;
    std::shared_ptr<LabelMap> label_mapping;

    Mapping() = default;

    Mapping (std::shared_ptr<StateMapping> sm,
             std::shared_ptr<LabelMap>  lm) :
    state_mapping (sm), label_mapping(lm) {
    }
};

enum class Verbosity {
    SILENT,
    NORMAL,
    VERBOSE
};
}

#endif
