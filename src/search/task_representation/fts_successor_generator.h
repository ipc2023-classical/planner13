#ifndef FTS_REPRESENTATION_FTS_SUCCESSOR_GENERATOR_H
#define FTS_REPRESENTATION_FTS_SUCCESSOR_GENERATOR_H

#include <memory>
#include <vector>

class GlobalState;
class OperatorID;

namespace task_representation {
class FTSTask;
class State;

class FTSSuccessorGenerator {
    
public:
    explicit FTSSuccessorGenerator(const FTSTask &task_proxy);
    /*
      We cannot use the default destructor (implicitly or explicitly)
      here because GeneratorBase is a forward declaration and the
      incomplete type cannot be destroyed.
    */
    ~FTSSuccessorGenerator();

    void generate_applicable_ops(
        const State &state, std::vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    void generate_applicable_ops(
        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const;
};
}

#endif
