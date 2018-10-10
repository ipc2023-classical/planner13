#include "relaxation_heuristic.h"

#include "../global_state.h"
#include "../globals.h"

#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../task_representation/transition_system.h"
#include "../task_representation/label_equivalence_relation.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <map>
#include <set>

using namespace std;

namespace relaxation_heuristic {

    //Auxiliary function to compute all combinations of preconditions. 
    void insert_all_combinations_recursive (const std::vector<std::vector<Proposition * > > & psets,
                                  std::vector<Proposition * > & new_combination,
                                  std::set<std::vector<Proposition *> > & result) {

        const auto &  pset = psets[new_combination.size()];
        for (Proposition * p : pset) {
            new_combination.push_back(p);
            if(new_combination.size() == psets.size()) {
                result.insert(new_combination);
            } else {
                insert_all_combinations_recursive(psets, new_combination, result);
            }
            new_combination.pop_back();
        }        
    }
    
    void insert_all_combinations (const std::vector<std::vector<Proposition * > > & psets,
                                  std::set<std::vector<Proposition *> > & result) {
        std::vector<Proposition * > new_combination;
        new_combination.reserve(psets.size());
        insert_all_combinations_recursive(psets, new_combination, result);
    }

    
// construction and destruction
RelaxationHeuristic::RelaxationHeuristic(const options::Options &opts)
    : Heuristic(opts) {
    // Build propositions.
    int prop_id = 0;
    propositions_per_var.resize(task->get_size());

    // auxiliary propositions represent a disjunction of facts that is used for (a)
    // goal condition (b) label precondition of labels with a precondition on more
    // than 2 variables
    vector<set<vector<int>>> auxiliary_subsets_per_var (task->get_size());
    vector<map<vector<int>, Proposition * >> auxiliary_propositions_per_var (task->get_size());

    // for each label with preconditions on more than 2 transition systems, we compute an
    // auxiliary proposition. 
    int num_labels = task->get_num_labels();
    for (task_representation::LabelID label_no(0); label_no < num_labels; ++label_no) {
        const auto & pre_transition_systems = task->get_label_preconditions(label_no);
        // We need to decide for which pairs transition-system/label we are going to add
        // such an auxiliary proposition
        double num_multiplied_operators = 1;
        vector<int> ts_with_multiple_pre; 
        for (int pre_ts : pre_transition_systems) {
            int applicable_in = task->get_ts(pre_ts).get_label_precondition(label_no).size();
            if (applicable_in > 1) {
                ts_with_multiple_pre.push_back(pre_ts);
                num_multiplied_operators *= applicable_in;
            }
        }

        //TODO: make parameter
        if (ts_with_multiple_pre.size() > 1 && num_multiplied_operators > 100) { 
            for (int pre_ts : ts_with_multiple_pre) {
                const auto & precond = task->get_ts(pre_ts).get_label_precondition(label_no);
                if (precond.size() > 2) { // TODO: make parameter
                    auxiliary_subsets_per_var[pre_ts].insert(precond);
                }
            }
        }
    }

    for (int lts_id = 0; lts_id < task->get_size(); ++lts_id){
        const auto & ts = task->get_ts(lts_id);
        const auto & goal_states = ts.get_goal_states();
        auto & propositions = propositions_per_var[lts_id];
        if (ts.is_goal_relevant() && goal_states.size() > 1) {
            auxiliary_subsets_per_var[lts_id].insert(goal_states);
        }
        
        propositions.reserve(ts.get_size() + auxiliary_subsets_per_var[lts_id].size());
        for (int s = 0; s < ts.get_size(); ++s) {
            propositions.push_back(Proposition(prop_id++));
        }

        for (const vector<int> & set_of_states : auxiliary_subsets_per_var[lts_id]) {
            propositions.push_back(Proposition(prop_id++));
            Proposition * aux_prop = &(propositions.back());
            auxiliary_propositions_per_var[lts_id][set_of_states] = aux_prop;

            vector<Proposition *> precondition;
            for (const auto & s : set_of_states) {
                precondition.push_back(&(propositions[s]));
            }
            unary_operators.push_back(UnaryOperator(precondition, aux_prop, -1, 0));
        }
        
        // Build goal propositions.
        if (ts.is_goal_relevant()) {
            const auto & goal_states = ts.get_goal_states(); 
            if (goal_states.size() == 1) {             
                goal_propositions.push_back(&(propositions[goal_states[0]]));
            } else {
                goal_propositions.push_back(auxiliary_propositions_per_var[lts_id][goal_states]);
            }
        }        
    }

    // Now we construct the unary operators for each label group, replacing preconditions
    // of labels by the auxiliary propositions
    for (int lts_id = 0; lts_id < task->get_size(); ++lts_id){
        const auto & ts = task->get_ts(lts_id);

        int op_no = 0; //TODO: Set op_no to get preferred operators
        
        // Build propositions for each label group in each transition system if it has relevant transitions
        for (const task_representation::GroupAndTransitions & gat : ts) {
            std::map<int, vector<int> > sources_by_target;
            for (const auto & tr : gat.transitions) {
                sources_by_target[tr.target].push_back(tr.src);
            }
            
            std::set<std::vector<Proposition *> > outside_conditions;
            for(task_representation::LabelID l : gat.label_group) {
                std::vector<std::vector<Proposition * > > pre_per_ts;
                const auto & pre_transition_systems = task->get_label_preconditions(l);
                for (int pre_ts : pre_transition_systems) {
                    const auto & pre = task->get_ts(pre_ts).get_label_precondition(l);
                    if (auxiliary_propositions_per_var[pre_ts].count(pre)){
                        pre_per_ts.push_back(vector<Proposition *> ());
                        pre_per_ts.back().push_back(auxiliary_propositions_per_var[pre_ts].at(pre));
                    } else {
                        pre_per_ts.push_back(vector<Proposition *> ());
                        pre_per_ts.back().reserve(pre.size());
                        for (int s : pre) {
                            pre_per_ts.back().push_back(&(propositions_per_var[pre_ts][s]));
                        }
                    }

                }
                
                insert_all_combinations(pre_per_ts, outside_conditions);
 
            }
            
            for (const auto & item  : sources_by_target) {
                int target = item.first;
                const vector<int> & sources = item.second;
                for (const auto & outside_condition : outside_conditions) {
                    if ((int)(sources.size()) == ts.get_size() ) {
                        unary_operators.push_back(UnaryOperator(outside_condition,
                                                                &(propositions_per_var[lts_id][target]),
                                                                op_no, gat.label_group.get_cost()));
                    } else {
                        auto pre = outside_condition; //copy 
                        pre.push_back(nullptr); // add dummy
                        for (int src : sources) {
                            //set dummy 
                            pre[pre.size() -1] = &(propositions_per_var[lts_id][src]);
                            unary_operators.push_back(UnaryOperator(pre,
                                                                    &(propositions_per_var[lts_id][target]),
                                                                    op_no, gat.label_group.get_cost()));
                        }
                    }
                }
            }
                
        }
    }





    
    // Simplify unary operators.
    simplify();

    // Cross-reference unary operators.
    for (size_t i = 0; i < unary_operators.size(); ++i) {
        UnaryOperator *op = &unary_operators[i];
        for (size_t j = 0; j < op->precondition.size(); ++j)
            op->precondition[j]->precondition_of.push_back(op);
    }
}

RelaxationHeuristic::~RelaxationHeuristic() {
}

bool RelaxationHeuristic::dead_ends_are_reliable() const {
    //return !has_axioms();
    return true;
}

// void RelaxationHeuristic::build_unary_operators(int label) {
//     int base_cost = op.get_cost();
//     vector<Proposition *> precondition_props; 
    
//     for (FactProxy precondition : op.get_preconditions()) {
//         precondition_props.push_back(get_proposition(precondition));
//     }

//     for (size_t i = 0; i < task->get_size(); ++i){
//         const auto & ts = task->get_ts(i);
//         ts.get_transitions_label();

//     }
    
//     for (EffectProxy effect : op.get_effects()) {
//         Proposition *effect_prop = get_proposition(effect.get_fact());
//         EffectConditionsProxy eff_conds = effect.get_conditions();
//         for (FactProxy eff_cond : eff_conds) {
//             precondition_props.push_back(get_proposition(eff_cond));
//         }
//         unary_operators.push_back(UnaryOperator(precondition_props, effect_prop, op_no, base_cost));
//         precondition_props.erase(precondition_props.end() - eff_conds.size(), precondition_props.end());
//     }
// }

void RelaxationHeuristic::simplify() {
    // Remove duplicate or dominated unary operators.

    /*
      Algorithm: Put all unary operators into an unordered map
      (key: condition and effect; value: index in operator vector.
      This gets rid of operators with identical conditions.

      Then go through the unordered map, checking for each element if
      none of the possible dominators are part of the map.
      Put the element into the new operator vector iff this is the case.

      In both loops, be careful to ensure that a higher-cost operator
      never dominates a lower-cost operator.

      In the end, the vector of unary operators is sorted by operator_no,
      effect->id, base_cost and precondition.
    */


    cout << "Simplifying " << unary_operators.size() << " unary operators..." << flush;

    typedef pair<vector<Proposition *>, Proposition *> Key;
    typedef unordered_map<Key, int> Map;
    Map unary_operator_index;
    unary_operator_index.reserve(unary_operators.size());


    for (size_t i = 0; i < unary_operators.size(); ++i) {
        UnaryOperator &op = unary_operators[i];
        sort(op.precondition.begin(), op.precondition.end(),
             [] (const Proposition *p1, const Proposition *p2) {
                return p1->id < p2->id;
            });
        Key key(op.precondition, op.effect);
        pair<Map::iterator, bool> inserted = unary_operator_index.insert(
            make_pair(key, i));
        if (!inserted.second) {
            // We already had an element with this key; check its cost.
            Map::iterator iter = inserted.first;
            int old_op_no = iter->second;
            int old_cost = unary_operators[old_op_no].base_cost;
            int new_cost = unary_operators[i].base_cost;
            if (new_cost < old_cost)
                iter->second = i;
            assert(unary_operators[unary_operator_index[key]].base_cost ==
                   min(old_cost, new_cost));
        }
    }

    vector<UnaryOperator> old_unary_operators;
    old_unary_operators.swap(unary_operators);

    for (Map::iterator it = unary_operator_index.begin();
         it != unary_operator_index.end(); ++it) {
        const Key &key = it->first;
        int unary_operator_no = it->second;
        bool match = false;
        if (key.first.size() <= 5) { // HACK! Don't spend too much time here...
            int powerset_size = (1 << key.first.size()) - 1; // -1: only consider proper subsets
            for (int mask = 0; mask < powerset_size; ++mask) {
                Key dominating_key = make_pair(vector<Proposition *>(), key.second);
                for (size_t i = 0; i < key.first.size(); ++i)
                    if (mask & (1 << i))
                        dominating_key.first.push_back(key.first[i]);
                Map::iterator found = unary_operator_index.find(
                    dominating_key);
                if (found != unary_operator_index.end()) {
                    int my_cost = old_unary_operators[unary_operator_no].base_cost;
                    int dominator_op_no = found->second;
                    int dominator_cost = old_unary_operators[dominator_op_no].base_cost;
                    if (dominator_cost <= my_cost) {
                        match = true;
                        break;
                    }
                }
            }
        }
        if (!match)
            unary_operators.push_back(old_unary_operators[unary_operator_no]);
    }

    sort(unary_operators.begin(), unary_operators.end(),
         [&] (const UnaryOperator &o1, const UnaryOperator &o2) {
            if (o1.operator_no != o2.operator_no)
                return o1.operator_no < o2.operator_no;
            if (o1.effect != o2.effect)
                return o1.effect->id < o2.effect->id;
            if (o1.base_cost != o2.base_cost)
                return o1.base_cost < o2.base_cost;
            return lexicographical_compare(o1.precondition.begin(), o1.precondition.end(),
                                           o2.precondition.begin(), o2.precondition.end(),
                                           [] (const Proposition *p1, const Proposition *p2) {
                return p1->id < p2->id;
            });
        });

    cout << " done! [" << unary_operators.size() << " unary operators]" << endl;
}
}
