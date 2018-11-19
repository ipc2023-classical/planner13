#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "evaluator.h"
#include "operator_id.h"
#include "per_state_information.h"

#include "algorithms/ordered_set.h"

#include <memory>
#include <vector>

class GlobalOperator;
class GlobalState;

namespace options {
class OptionParser;
class Options;
}

namespace task_representation {
class State;
class SearchTask;
}


class Heuristic : public Evaluator {
    struct HEntry {
        /* dirty is conceptually a bool, but Visual C++ does not support
           packing ints and bools together in a bitfield. */
        int h : 31;
        unsigned int dirty : 1;

        HEntry(int h, bool dirty)
            : h(h), dirty(dirty) {
        }
    };
    static_assert(sizeof(HEntry) == 4, "HEntry has unexpected size.");

    std::string description;

    PreferredOperatorsInfo preferred_operators;

protected:
    /*
      Cache for saving h values
      Before accessing this cache always make sure that the cache_h_values
      flag is set to true - as soon as the cache is accessed it will create
      entries for all existing states
    */
    PerStateInformation<HEntry> heuristic_cache;
    bool cache_h_values;

    // Hold a reference to the task implementation and pass it to objects that need it.
    std::shared_ptr<task_representation::FTSTask> task;
    std::shared_ptr<task_representation::SearchTask> search_task;

    OperatorCost cost_type;

    enum {DEAD_END = -1, NO_VALUE = -2};

    // TODO: Call with State directly once all heuristics support it.
    virtual int compute_heuristic(const GlobalState &state) = 0;

    /*
      Usage note: Marking the same operator as preferred multiple times
      is OK -- it will only appear once in the list of preferred
      operators for this heuristic.
    */
    void set_preferred(int label, const task_representation::FactPair & fact);
    

    task_representation::State convert_global_state(const GlobalState &global_state) const;

public:
    explicit Heuristic(const options::Options &options);
    virtual ~Heuristic() override;

    virtual void notify_initial_state(const GlobalState & /*initial_state*/) {
    }

    virtual bool notify_state_transition(
        const GlobalState &parent_state, const OperatorID op,
        const GlobalState &state);

    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override {
        hset.insert(this);
    }

    int get_label_cost(int label) const;
    
    static void add_options_to_parser(options::OptionParser &parser);
    static options::Options default_options();

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    std::string get_description() const;
};

#endif
