#ifndef PRUNING_METHOD_H
#define PRUNING_METHOD_H

#include "operator_id.h"

#include <memory>
#include <vector>

class FTSTask;
class GlobalOperator;
class GlobalState;
class State;
class PruningMethod {
protected:
    std::shared_ptr<FTSTask> task;

public:
    PruningMethod();
    virtual ~PruningMethod() = default;

    virtual void initialize(const std::shared_ptr<FTSTask> &task);

    /* This method must not be called for goal states. This can be checked
       with assertions in derived classes. */
    virtual void prune_operators(const State &state,
                                 std::vector<OperatorID> &op_ids) = 0;
    // TODO remove this overload once the search uses the task interface.
    virtual void prune_operators(const GlobalState &state,
                                 std::vector<OperatorID> &op_ids);

    virtual void print_statistics() const = 0;
};

#endif
