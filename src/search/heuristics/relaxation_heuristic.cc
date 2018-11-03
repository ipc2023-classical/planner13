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

using task_representation::LabelID;
using task_representation::FTSTask;
using task_representation::FactPair;

namespace relaxation_heuristic {


    void insert_outside_condition(LabelID l, const FTSTask * task,
				  std::map<std::vector<Proposition *>, LabelID> & result,
				  const std::vector<Proposition * > & new_outside_condition){

	auto pos = result.lower_bound(new_outside_condition);

	if(pos != result.end() && !(result.key_comp()(new_outside_condition, pos->first))) {
	    // key already exists, update if the new label has lower cost
	    if (task->get_label_cost(pos->second)  > task->get_label_cost(l)) {
		pos->second = l;
	    }
	} else {
	    result.insert(pos, std::map<std::vector<Proposition *>, LabelID>::value_type(new_outside_condition, l));
	}	
    }
    //Auxiliary function to compute all combinations of preconditions. 
    void insert_all_combinations_recursive (LabelID l, const FTSTask * task, 
					    const std::vector<std::vector<Proposition * > > & psets,
					    std::vector<Proposition * > & new_combination,
					    std::map<std::vector<Proposition *>, LabelID> & result) {

        const auto &  pset = psets[new_combination.size()];
        for (Proposition * p : pset) {
            new_combination.push_back(p);
            if(new_combination.size() == psets.size()) {
		insert_outside_condition(l, task, result, new_combination);
	    } else {
                insert_all_combinations_recursive(l, task, psets, new_combination, result);
            }
            new_combination.pop_back();
        }        
    }
    
    void insert_all_combinations (LabelID l, FTSTask * task,
				  const std::vector<std::vector<Proposition * > > & psets,
                                  std::map<std::vector<Proposition *> , LabelID> & result) {
	assert(!psets.empty()) ;
	std::vector<Proposition * > new_combination;
	new_combination.reserve(psets.size());
	insert_all_combinations_recursive(l, task, psets, new_combination, result);    
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
            unary_operators.push_back(UnaryOperator(precondition, aux_prop, RelaxedPlanStep(), 0));
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
        // Build propositions for each label group in each transition system if it has relevant transitions
        for (const task_representation::GroupAndTransitions & gat : ts) {
            std::map<int, vector<int> > sources_by_target;
            for (const auto & tr : gat.transitions) {
		if (tr.src != tr.target) {
		    sources_by_target[tr.target].push_back(tr.src);
		}
            }
            
	    std::map<std::vector<Proposition *> , LabelID>  outside_conditions;
            for(int l : gat.label_group) {
                std::vector<std::vector<Proposition * > > pre_per_ts;
                const auto & pre_transition_systems = task->get_label_preconditions(l);

		if (pre_transition_systems.empty() ||
		    (pre_transition_systems.size() == 1 && pre_transition_systems[0] == lts_id)) {
		    insert_outside_condition(LabelID(l), task.get(), outside_conditions, std::vector<Proposition *>());
		} else {
		    for (int pre_ts : pre_transition_systems) {
			if (pre_ts == lts_id) {
			    continue;
			}
			const auto & pre = task->get_ts(pre_ts).get_label_precondition(LabelID(l));
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
                
		    insert_all_combinations(LabelID(l), task.get(), pre_per_ts, outside_conditions);
		}
            }
            
            for (const auto & item  : sources_by_target) {
                int target = item.first;
                const vector<int> & sources = item.second;
                for (const auto & outside_condition : outside_conditions) {
		    RelaxedPlanStep rs_step (outside_condition.second, FactPair(lts_id, target));
                    if ((int)(sources.size()) == ts.get_size() - 1 ) {
                        unary_operators.push_back(UnaryOperator(outside_condition.first,
                                                                &(propositions_per_var[lts_id][target]),
                                                                rs_step,
								task->get_label_cost(outside_condition.second)));
			// for (OperatorID op_id : rs_step.get_operator_ids()) {
			//     unary_operators_per_operator_id[op_id].push_back(&(unary_operators.back()));
			// }
                    } else {
                        auto pre = outside_condition.first; //copy
                        pre.push_back(nullptr); // add dummy
                        for (int src : sources) {
			    assert (src != target);
                            //set dummy 
                            pre[pre.size() -1] = &(propositions_per_var[lts_id][src]);
                            unary_operators.push_back(UnaryOperator(pre,
                                                                    &(propositions_per_var[lts_id][target]),
                                                                    rs_step,
								    task->get_label_cost(outside_condition.second)));

			    // for (OperatorID op_id : rs_step.get_operator_ids()) {
			    // 	unary_operators_per_operator_id[op_id].push_back(&(unary_operators.back()));
			    // }
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
	     if (o1.rp_step.label != o2.rp_step.label)
		 return o1.rp_step.label < o2.rp_step.label;
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
