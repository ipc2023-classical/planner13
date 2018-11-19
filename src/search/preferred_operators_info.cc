#include "preferred_operators_info.h"

#include "task_representation/search_task.h"
#include "task_transformation/label_map.h"
#include "task_transformation/state_mapping.h"

using namespace task_representation;
using namespace task_transformation;
using namespace std;

void PreferredOperatorsInfo::clear() {
    preferred_effects_by_label.clear();
}

void PreferredOperatorsInfo::set_preferred(int label, const task_representation::FactPair & fact) {
    preferred_effects_by_label[label].push_back(fact);
}

void PreferredOperatorsInfo::get_preferred_operators(const Mapping & mapping,
                                                     const GlobalState & state,
                                                     const SearchTask & search_task, 
                                                     const std::vector<OperatorID> & applicable_operators,
                                                     ordered_set::OrderedSet<OperatorID> & preferred_operators) const{

    if (mapping.label_mapping) {
        for (OperatorID op_id : applicable_operators) {
            int label = mapping.label_mapping->get_reduced_label(search_task.get_label(op_id));
            auto effects_by_label = preferred_effects_by_label.find(label);
            if (effects_by_label == preferred_effects_by_label.end()) {
                continue;
            }
            vector<int> state_values = state.get_values();
            search_task.apply_operator(state, op_id, state_values);
                
            for (const FactPair & effect : effects_by_label->second) {
                //We need to figure out whether applying the operator on state will result in
                //the relevant effect
                if(mapping.state_mapping->get_value_abstract_variable(state_values, effect.var)
                   == effect.value) {
                    preferred_operators.insert(op_id);
                    break; 
                }
            }
        }
    }else {
        for (OperatorID op_id : applicable_operators) {
            int label = search_task.get_label(op_id);
            auto effects_by_label = preferred_effects_by_label.find(label);
            if (effects_by_label == preferred_effects_by_label.end()) {
                continue;
            }
        
            for (const FactPair & effect : effects_by_label->second) {
                //We need to figure out whether applying the operator on state will result in
                //the relevant effect

                if(search_task.has_effect(state, op_id, effect)) {
                    preferred_operators.insert(op_id);
                    break; 
                }
            }
        }
    }
    
    
}

