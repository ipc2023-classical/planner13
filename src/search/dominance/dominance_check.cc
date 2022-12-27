#include "dominance_check.h"

#include "../task_representation/search_task.h"
using namespace std;
using namespace task_representation;

namespace dominance {

    template<typename TCost>
    void DominanceCheck<TCost>::initialize(std::shared_ptr<DominanceFunction<TCost>> _qdf, const FTSTask & task) {
        qdf = _qdf;
        parent.resize(task.get_size());
        succ.resize(task.get_size());
        values_initial_state_against_parent.resize(task.get_size());
    }

    template<typename TCost>
    bool DominanceCheck<TCost>::strictly_dominates_initial_state(const State &t) const {
        //TODO: Avoid unnecessary copy here. This is only needed for transformations
        vector<int> copy_t(qdf->size());
        for (size_t i = 0; i < qdf->size(); ++i) {
            copy_t[i] = t[i];
        }
        return qdf->dominates_parent(copy_t, initial_state, 0) && !qdf->dominates_parent(initial_state, copy_t, 0);
    }


    template<typename TCost>
    void DominanceCheck<TCost>::prune_dominated_by_parent_or_initial_state(const FTSTask & fts_task,
                                                                          const State &state,
                                                                          std::vector<OperatorID> &applicable_operators,
                                                                          bool parent_ids_stored,
                                                                          bool compare_against_parent,
                                                                          bool compare_against_initial_state) const {

        const shared_ptr<SearchTask> &search_task = fts_task.get_search_task();

        if (!parent_ids_stored) {
            for (size_t i = 0; i < qdf->size(); ++i) {
                succ[i] = state[i];
            }
            if (compare_against_parent) {
                parent = succ;
            }
        }

        vector<int> ts_initial_state_does_not_simulate_parent;
        TCost initial_state_against_parent = 0;
        if (compare_against_initial_state) {
            for (size_t i = 0; i < qdf->size(); ++i) {
                values_initial_state_against_parent[i] =
                        (*qdf)[i].q_simulates(initial_state[i], parent[i]);
                if (values_initial_state_against_parent[i] == std::numeric_limits<int>::lowest()) {
                    ts_initial_state_does_not_simulate_parent.push_back(int(i));
                } else {
                    initial_state_against_parent += values_initial_state_against_parent[i];
                }
            }
        }

        int detected_dead_ends = 0;
//    int ops_before = int(applicable_operators.size());
        applicable_operators.erase(std::remove_if(applicable_operators.begin(),
                                                  applicable_operators.end(),
                                                  [&](const OperatorID &op_id) {
                                                      FTSOperator fts_op = search_task->get_fts_operator(op_id);

                                                      succ = search_task->generate_successor(state, op_id);

                                                      for (size_t i = 0; i < succ.size(); i++) {
                                                          if (succ[i] != parent[i])
                                                              relevant_simulations.insert(int(i));
                                                      }

                                                      bool proved_prunable = false;

                                                      //Check dead_ends
                                                      for (int sim: relevant_simulations) {
                                                          if (succ[sim] == -1) {
                                                              detected_dead_ends++;
                                                              proved_prunable = true;
                                                          }
                                                      }

                                                      if (!proved_prunable && compare_against_parent) {
                                                          TCost total_value = 0;
                                                          bool may_simulate = true;
                                                          for (int sim: relevant_simulations) {
                                                              TCost val = (*qdf)[sim].q_simulates(parent[sim],succ[sim]);

                                                              if (val == std::numeric_limits<int>::lowest()) {
                                                                  may_simulate = false;
                                                                  break;
                                                              }
                                                              total_value += val;
                                                          }

                                                          //TODO: Use adjusted_cost instead?
                                                          proved_prunable = may_simulate && (total_value >= 0 ||
                                                                                             total_value +
                                                                                             fts_op.get_cost() > 0);
                                                      }

                                                      if (!proved_prunable && compare_against_initial_state
                                                          && ts_initial_state_does_not_simulate_parent.size() <=
                                                             relevant_simulations.size()) {

                                                          bool all_not_simulated_change = true;
                                                          for (int sim_must_change: ts_initial_state_does_not_simulate_parent) {
                                                              bool found = false;
                                                              for (int sim: relevant_simulations) {
                                                                  if (sim_must_change == sim) {
                                                                      found = true;
                                                                      break;
                                                                  }
                                                              }
                                                              if (!found) {
                                                                  all_not_simulated_change = false;
                                                                  break; //proved no
                                                              }
                                                          }

                                                          if (all_not_simulated_change) {
                                                              TCost total_value = initial_state_against_parent;
                                                              bool may_simulate = true;
                                                              for (int sim: relevant_simulations) {
                                                                  TCost val = (*qdf)[sim].q_simulates(initial_state[sim], succ[sim]);
                                                                  if (val == std::numeric_limits<int>::lowest()) {
                                                                      may_simulate = false;
                                                                      break;
                                                                  }
                                                                  total_value += val;
                                                                  if (values_initial_state_against_parent[sim] !=
                                                                      std::numeric_limits<int>::lowest()) {
                                                                      total_value -= values_initial_state_against_parent[sim];
                                                                  }
                                                              }
                                                              proved_prunable = may_simulate && (total_value >= 0 ||
                                                                                                 total_value +
                                                                                                 fts_op.get_cost() > 0);
                                                          }
                                                      }

                                                      relevant_simulations.clear();

                                                      return proved_prunable;
                                                  }), applicable_operators.end());



//    if (ops_before > applicable_operators.size()) {
//        search_progress.inc_dead_ends(detected_dead_ends);
//        search_progress.inc_pruned((ops_before - applicable_operators.size()) - detected_dead_ends);
//        //cout << "Pruned "  << ops_before  -applicable_operators.size() << " out of " << ops_before << endl;
//    }
    }

    template<typename TCost>
    bool DominanceCheck<TCost>::action_selection_pruning(const FTSTask & fts_task, const State &state,
                                                        std::vector<OperatorID> &applicable_operators) const {
        const shared_ptr<SearchTask> &search_task = fts_task.get_search_task();
        assert(qdf);
        //TODO: Apply transformation if needed
        for (size_t i = 0; i < qdf->size(); ++i) {
            parent[i] = state[i];
        }

        succ = parent;
        for (auto op_id: applicable_operators) {
            FTSOperator fts_op = search_task->get_fts_operator(op_id);

            //TODO: If operator (both op_id) touches a "forbidden" variable/value, then we can insert it in the list of applicable_operators and  skip the rest.
            // We can precompute such a list of relevant operators. A forbidden variable is one where there
            // is no possibility of finding any dominance. A forbidden value is a refinement of that.
            succ = search_task->generate_successor(state, op_id);

            for (size_t i = 0; i < succ.size(); i++) {
                if (succ[i] != parent[i])
                    relevant_simulations.insert(int(i));
            }

            TCost total_value = 0;
            bool may_simulate = true;
            for (int sim: relevant_simulations) {
                // TODO: Succ_id is the same as succ as there is no transformation on the task
                int succ_id = succ[(*qdf)[sim].get_ts_id()];
                if (succ_id == -1) {
                    may_simulate = false;
                    break;
                }
                TCost val = (*qdf)[sim].q_simulates(succ_id, parent[sim]);
                if (val == std::numeric_limits<int>::lowest()) {
                    may_simulate = false;
                    break;
                }
                total_value += val;
            }
            relevant_simulations.clear();

            //TODO: Use adjusted cost instead.
            if (may_simulate && total_value - fts_op.get_cost() >= 0) {
                applicable_operators.clear();
                applicable_operators.push_back(op_id);
                return true;
            }

        }

        return false;
    }



    template
    class DominanceCheck<int>;

    template
    class DominanceCheck<IntEpsilon>;

}