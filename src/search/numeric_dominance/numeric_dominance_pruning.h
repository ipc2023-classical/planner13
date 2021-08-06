#ifndef NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_PRUNING_H
#define NUMERIC_DOMINANCE_NUMERIC_DOMINANCE_PRUNING_H

#include "../prune_heuristic.h"

#include "tau_labels.h"
#include "int_epsilon.h"
#include "numeric_dominance_relation.h"

class LDSimulation;
class Abstraction;

template <typename T>
class NumericDominancePruning : public PruneHeuristic {
 protected:

  bool initialized;
  std::shared_ptr<TauLabelManager<T>> tau_labels;

  const bool prune_dominated_by_parent;
  const bool prune_dominated_by_initial_state;
  const bool prune_successors;

  const int truncate_value;
  const int max_simulation_time;
  const int min_simulation_time;
  const int max_total_time;

  const int max_lts_size_to_compute_simulation;
  const int num_labels_to_use_dominates_in;

  const bool dump;
  const bool exit_after_preprocessing;

  std::unique_ptr<LDSimulation> ldSimulation;
  std::unique_ptr<NumericDominanceRelation<T>> numeric_dominance_relation;
  std::vector<std::unique_ptr<Abstraction> > abstractions;

  bool all_desactivated;
  bool activation_checked;

  int states_inserted; //Count the number of states inserted
  int states_checked; //Count the number of states inserted
  int states_pruned; //Count the number of states pruned
  int deadends_pruned; //Count the number of dead ends detected

  void dump_options() const;

  bool apply_pruning() const;

 public:
  virtual void initialize(bool force_initialization = false) override;

  //Methods for pruning explicit search
  virtual void prune_applicable_operators(const State & state, int g, std::vector<const Operator *> & operators, SearchProgress & search_progress) override;
  virtual bool prune_generation(const State &state, int g, const State &parent, int action_cost) override;
  virtual bool prune_expansion (const State &state, int g) override;

  virtual bool is_dead_end(const State &state) override;

  virtual int compute_heuristic(const State &state) override;

  NumericDominancePruning(const Options &opts);
  virtual ~NumericDominancePruning() = default;

  virtual void print_statistics() override;

  virtual bool proves_task_unsolvable() const override {
      return true;
  }
};



#endif
