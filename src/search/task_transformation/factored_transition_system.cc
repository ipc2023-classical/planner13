#include "factored_transition_system.h"

#include "distances.h"
#include "merge_and_shrink_representation.h"
#include "utils.h"
#include "plan_reconstruction_merge_and_shrink.h"

#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "../task_representation/label_equivalence_relation.h"

#include "../utils/collections.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include "label_map.h"

#include <cassert>

using namespace std;

namespace task_transformation {
FTSConstIterator::FTSConstIterator(
    const FactoredTransitionSystem &fts,
    bool end)
    : fts(fts), current_index((end ? fts.get_size() : 0)) {
    next_valid_index();
}

void FTSConstIterator::next_valid_index() {
    while (current_index < fts.get_size()
           && !fts.is_active(current_index)) {
        ++current_index;
    }
}

void FTSConstIterator::operator++() {
    ++current_index;
    next_valid_index();
}


FactoredTransitionSystem::FactoredTransitionSystem(
    std::shared_ptr<task_representation::FTSTask> fts_task, 
    unique_ptr<Labels> labels_,
    vector<unique_ptr<TransitionSystem>> &&transition_systems,
    vector<unique_ptr<MergeAndShrinkRepresentation>> &&mas_representations,
    vector<unique_ptr<Distances>> &&distances,
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity)
    : labels(move(labels_)),
      transition_systems(move(transition_systems)),
      mas_representations(move(mas_representations)),
      label_map (new LabelMap (labels->get_size())),
      distances(move(distances)),
      compute_init_distances(compute_init_distances),
      compute_goal_distances(compute_goal_distances),
      num_active_entries(this->transition_systems.size()),
      predecessor_fts_task (fts_task){
    for (size_t index = 0; index < this->transition_systems.size(); ++index) {
        if (compute_init_distances || compute_goal_distances) {
            this->distances[index]->compute_distances(
                compute_init_distances, compute_goal_distances, verbosity);
        }
        assert(is_component_valid(index));
    }
}

FactoredTransitionSystem::FactoredTransitionSystem(FactoredTransitionSystem &&other)
    : labels(move(other.labels)),
      transition_systems(move(other.transition_systems)),
      mas_representations(move(other.mas_representations)),
      label_map(move(other.label_map)),
      distances(move(other.distances)),
      compute_init_distances(move(other.compute_init_distances)),
      compute_goal_distances(move(other.compute_goal_distances)),
      num_active_entries(move(other.num_active_entries)) {
    /*
      This is just a default move constructor. Unfortunately Visual
      Studio does not support "= default" for move construction or
      move assignment as of this writing.
    */
}

FactoredTransitionSystem::~FactoredTransitionSystem() {
}

bool FactoredTransitionSystem::apply_abstraction(
    int index,
    const StateEquivalenceRelation &state_equivalence_relation,
    Verbosity verbosity, bool ignore_mas_representation) {
    
    if (!ignore_mas_representation) {
        assert(is_component_valid(index));
    }

    int new_num_states = state_equivalence_relation.size();
    if (new_num_states == transition_systems[index]->get_size()) {
        if (verbosity >= Verbosity::VERBOSE) {
            cout << transition_systems[index]->tag()
                 << "not applying abstraction (same number of states)" << endl;
        }
        return false;
    }

    vector<int> abstraction_mapping = compute_abstraction_mapping(
        transition_systems[index]->get_size(), state_equivalence_relation);

    transition_systems[index]->apply_abstraction(
        state_equivalence_relation, abstraction_mapping, verbosity);
    if (compute_init_distances || compute_goal_distances) {
        distances[index]->apply_abstraction(
            state_equivalence_relation,
            compute_init_distances,
            compute_goal_distances,
            verbosity);
    }

    if (!ignore_mas_representation) {
        mas_representations[index]->apply_abstraction_to_lookup_table(
            abstraction_mapping);

        /* If distances need to be recomputed, this already happened in the
           Distances object. */
        assert(is_component_valid(index));
    }
    return true;
}

void FactoredTransitionSystem::assert_index_valid(int index) const {
    assert(utils::in_bounds(index, transition_systems));
    assert(utils::in_bounds(index, mas_representations));
    assert(utils::in_bounds(index, distances));
    if (!(transition_systems[index] && mas_representations[index] && distances[index]) &&
        !(!transition_systems[index] && !mas_representations[index] && !distances[index])) {
        cerr << "Factor at index is in an inconsistent state!" << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
    }
}

bool FactoredTransitionSystem::is_component_valid(int index) const {
    assert(is_active(index));
    return /*distances[index]->are_distances_computed()
           && */transition_systems[index]->are_transitions_sorted_unique();
}

void FactoredTransitionSystem::assert_all_components_valid() const {
    for (size_t index = 0; index < transition_systems.size(); ++index) {
        if (transition_systems[index]) {
            assert(is_component_valid(index));
        }
    }
}

void FactoredTransitionSystem::apply_label_mapping(
    const vector<pair<int, vector<int>>> &label_mapping,
    int combinable_index) {
    assert_all_components_valid();

    vector<int> old_to_new_labels(labels->get_size(), -1);
    for (const auto &new_label_old_labels : label_mapping) {
        assert(new_label_old_labels.first == labels->get_size());
        labels->reduce_labels(new_label_old_labels.second);

        int next_new_label_no = new_label_old_labels.first;
        for (int old_label_no : new_label_old_labels.second) {
            old_to_new_labels[old_label_no] = next_new_label_no;
        }

    }

    label_map->update(old_to_new_labels);

    for (size_t i = 0; i < transition_systems.size(); ++i) {
        if (transition_systems[i]) {
            transition_systems[i]->apply_label_reduction(
                label_mapping, static_cast<int>(i) != combinable_index);
        }
    }
    assert_all_components_valid();

}

int FactoredTransitionSystem::merge(
    int index1,
    int index2,
    Verbosity verbosity) {
    assert(is_component_valid(index1));
    assert(is_component_valid(index2));
    transition_systems.push_back(
        TransitionSystem::merge(
            *labels,
            *transition_systems[index1],
            *transition_systems[index2],
            verbosity));
    distances[index1] = nullptr;
    distances[index2] = nullptr;
    transition_systems[index1] = nullptr;
    transition_systems[index2] = nullptr;
    mas_representations.push_back(
        utils::make_unique_ptr<MergeAndShrinkRepresentationMerge>(
            move(mas_representations[index1]),
            move(mas_representations[index2])));
    mas_representations[index1] = nullptr;
    mas_representations[index2] = nullptr;
    const TransitionSystem &new_ts = *transition_systems.back();
    distances.push_back(utils::make_unique_ptr<Distances>(new_ts));
    int new_index = transition_systems.size() - 1;
    // Restore the invariant that distances are computed.
    if (compute_init_distances || compute_goal_distances) {
        distances[new_index]->compute_distances(
            compute_init_distances, compute_goal_distances, verbosity);
    }
    --num_active_entries;
    assert(is_component_valid(new_index));
    return new_index;
}

// unique_ptr<TransitionSystem> FactoredTransitionSystem::extract_transition_system(int index) {
//     // This assertion does not hold since we don't extract both transition
//     // system and merge-and-shrink representation at the same time.
// //    assert(is_active(index));
//     return move(transition_systems[index]);
// }

    vector<unique_ptr<TransitionSystem>> FactoredTransitionSystem::extract_transition_systems() {
        return move(transition_systems);
    }

unique_ptr<MergeAndShrinkRepresentation> FactoredTransitionSystem::extract_mas_representation(int index) {
    // This assertion does not hold since we don't extract both transition
    // system and merge-and-shrink representation at the same time.
//    assert(is_active(index));
    return move(mas_representations[index]);
}

unique_ptr<Labels> FactoredTransitionSystem::extract_labels() {
    return move(labels);
}

void FactoredTransitionSystem::statistics(int index) const {
    assert(is_component_valid(index));
    const TransitionSystem &ts = *transition_systems[index];
    ts.statistics();
    const Distances &dist = *distances[index];
    dist.statistics();
}

void FactoredTransitionSystem::dump(int index) const {
    assert_index_valid(index);
    transition_systems[index]->dump_labels_and_transitions();
    mas_representations[index]->dump();
}

bool FactoredTransitionSystem::is_factor_solvable(int index) const {
    assert(is_component_valid(index));
    return transition_systems[index]->is_solvable(*distances[index]);
}

bool FactoredTransitionSystem::is_active(int index) const {
    assert_index_valid(index);
    return transition_systems[index] != nullptr;
}
  

bool FactoredTransitionSystem::remove_irrelevant_transition_systems(Verbosity verbosity) {
    bool removed_tr = false;
    for (size_t index = 0; index < transition_systems.size(); ++index) {
        if (transition_systems[index] && transition_systems[index]->get_size() <= 1) {
            transition_systems[index] = nullptr;
            distances[index] = nullptr;
            mas_representations[index] = nullptr;
            --num_active_entries;
            removed_tr = true;
            if (verbosity >= Verbosity::VERBOSE) {
                cout << "removing irrelevant TS at index " << index << endl;
            }
        }
    }
    return removed_tr;
}
    
    bool  FactoredTransitionSystem::is_irrelevant_label (LabelID label) const {
        for (const auto & ts : transition_systems) {
            if(ts && ts->is_relevant_label(label)) {
                return false;
            }
        }
        return true;
    }
    
bool FactoredTransitionSystem::remove_irrelevant_labels () {
    std::vector<LabelID> irrelevant_labels;
    for (LabelID (label_no); label_no < labels->get_size(); ++label_no) {         
        if (labels->is_current_label(label_no) && is_irrelevant_label(label_no)) {
            irrelevant_labels.push_back(label_no);
        }
    }
    
    labels->remove_labels(irrelevant_labels);

    bool may_require_pruning = false;
    for (size_t i = 0; i < transition_systems.size(); ++i) {
        if (transition_systems[i]) {
            may_require_pruning |= transition_systems[i]->remove_labels(irrelevant_labels);
        }
    }

    assert(!may_require_pruning);

    return !irrelevant_labels.empty();
}


vector<LabelID> FactoredTransitionSystem::get_tau_labels (int index) const{
   vector<LabelID> tau_labels;
   for (LabelGroupID relevant_group : transition_systems[index]->get_relevant_label_groups()) {            for (int l : transition_systems[index]->get_label_group(relevant_group)) {
           if (is_tau_label(index, LabelID(l))) {
               tau_labels.push_back(LabelID(l));
           }
       }
   }
   
   return tau_labels;
}

   
    bool FactoredTransitionSystem::is_tau_label (int ts_index, LabelID label) const{

        // cout << "CHECKING TAU " << ts_index << " " << label << endl;
        for (size_t index = 0; index < transition_systems.size(); ++index) {
            if (transition_systems[index] && (int)index != ts_index &&
                !transition_systems[index]->is_selfloop_everywhere(label)) {
                // cout << label << " is not tau for " << ts_index << " because of " << index <<endl;
                return false;
            }
        }

        //cout << label << " TAU TAU TAU " << ts_index << endl;
        
        return true;
    }

    bool FactoredTransitionSystem::is_only_goal_relevant (int ts_index) const {

        for (size_t i = 0; i < transition_systems.size() ; ++i) {
            if (transition_systems[i]) {
                if ((int)i == ts_index){
                    if (!transition_systems[i]->is_goal_relevant()) {
                        return false;
                    }
                } else {
                    if (transition_systems[i]->is_goal_relevant()) {
                        return false;
                    }
                }
            }
        }
        return true;
    }


    // void FactoredTransitionSystem::cleanup(bool continue_mas_process) {
    //     // "Renumber" factors consecutively. (Actually, nothing to do except storing them
    //     // consecutively since factor indices are not stored anywhere.)
    //     //cout << "Number of remaining factors: " << num_active_entries << endl;

    //     // Renumber labels consecutively
    //     int new_num_labels = labels->get_num_active_entries();
    //     cout << "Number of remaining labels: " << new_num_labels << endl;
    //     vector<unique_ptr<task_representation::Label>> active_labels;
    //     vector<int> old_to_new_labels(labels->get_size(), -1);
    //     active_labels.reserve(new_num_labels);
    //     for (int label_no = 0; label_no < labels->get_size(); ++label_no) {
    //         if (labels->is_current_label(label_no)) {
    //             int new_label_no = active_labels.size();
    //             active_labels.push_back(labels->extract_label(label_no));
    //             old_to_new_labels[label_no] = new_label_no;
    //         }
    //     }
        
    //     cout << "Renumbering labels: " << endl;// old_to_new_labels << endl;
    //     for (unique_ptr<TransitionSystem> &ts : transition_systems) {
    //         if (ts) {
    //             ts->renumber_labels(old_to_new_labels, new_num_labels);
    //         }
    //     }


    //     vector<unique_ptr<TransitionSystem> > new_transition_systems;
    //     new_transition_systems.reserve(num_active_entries);
    //     vector<unique_ptr<Distances> > new_distances;
    //     new_distances.reserve(num_active_entries);

    //     vector<unique_ptr<MergeAndShrinkRepresentation> > old_mas_representations;        
    //     old_mas_representations.reserve(num_active_entries);

    //     int space_for_labels = continue_mas_process ?
    //         active_labels.size()*2-1 :
    //         active_labels.size();

    //     unique_ptr<Labels> old_labels = move(labels);
    //     labels.reset(new Labels(move(active_labels), space_for_labels));

    //     for (int ts_index : *this) {
    //         if (is_active(ts_index)) {
    //             new_transition_systems.
    //                 push_back(utils::make_unique_ptr<TransitionSystem>(*transition_systems[ts_index], *labels));
    //             new_distances.push_back(utils::make_unique_ptr<Distances>(*(new_transition_systems.back())));

    //             new_distances.back()->compute_distances(
    //                 compute_init_distances, compute_goal_distances, Verbosity::NORMAL);

    //             old_mas_representations.push_back(extract_mas_representation(ts_index));
    //         }
    //     }
        
    //     transition_systems.swap(new_transition_systems);
    //     distances.swap(new_distances);
        
    //     cout << "Done renumbering factors." << transition_systems.size() << endl;

    //     cout << "Update label map" << endl;
    //     label_map->update(old_to_new_labels);
    //     auto old_label_map = move(label_map);
    //     cout << "Done renumbering labels." << endl;

    //     plan_reconstruction_steps.push_back(
    //         make_shared<PlanReconstructionMergeAndShrink>(
    //             predecessor_fts_task, move(old_mas_representations),
    //             move(old_label_map)));
 
    //     if (continue_mas_process) {            
    //         mas_representations.clear();
    //         for (size_t index = 0; index < transition_systems.size(); ++index) {
    //             mas_representations.push_back(
    //                 utils::make_unique_ptr<MergeAndShrinkRepresentationLeaf>
    //                 (index, transition_systems[index]->get_size()));
    //         }

    //         label_map.reset(new LabelMap (labels->get_size()));




    //         // new_transition_systems.clear();
    //         // for(auto & ts : transition_systems ) {
    //         //     new_transition_systems.push_back(utils::make_unique_ptr<TransitionSystem> (*ts));
    //         // }
            
    //         // predecessor_fts_task = make_shared<task_representation::FTSTask>
    //         //     (move(new_transition_systems), utils::make_unique_ptr<Labels>(*labels));
    //     }
    // }




    void FactoredTransitionSystem::cleanup() {
        // "Renumber" factors consecutively. (Actually, nothing to do except storing them
        // consecutively since factor indices are not stored anywhere.)
        //cout << "Number of remaining factors: " << num_active_entries << endl;

        // 1) Construct plan reconstruction object       
        vector<unique_ptr<TransitionSystem> > new_transition_systems;
        vector<unique_ptr<Distances> > new_distances;

        new_transition_systems.reserve(num_active_entries);
        vector<unique_ptr<MergeAndShrinkRepresentation> > old_mas_representations;        
        old_mas_representations.reserve(num_active_entries);
        for (int ts_index : *this) {
            if (is_active(ts_index)) {
                new_transition_systems.push_back(move(transition_systems[ts_index]));
                old_mas_representations.push_back(extract_mas_representation(ts_index));
                new_distances.push_back(move(distances[ts_index]));
            }
        }
        transition_systems.swap(new_transition_systems);
        distances.swap(new_distances);
        cout << "Done renumbering factors." << endl;
        // Renumber labels consecutively
        int new_num_labels = labels->get_num_active_entries();
        // cout << "Number of remaining labels: " << new_num_labels << endl;
        vector<int> old_to_new_labels = labels->cleanup();
        assert(new_num_labels == labels->get_size());

        cout << "Renumbering labels: " << endl;// old_to_new_labels << endl;
        for (unique_ptr<TransitionSystem> &ts : transition_systems) {
             ts->renumber_labels(old_to_new_labels, new_num_labels);
         }
        // cout << "Update label map" << endl;
         label_map->update(old_to_new_labels);

        // //labels = utils::make_unique_ptr<Labels>(move(active_labels), space_for_labels );
        // cout << "Done renumbering labels." << endl;
        //label_map->dump();

        plan_reconstruction_steps.push_back(
            make_shared<PlanReconstructionMergeAndShrink>(
                predecessor_fts_task, move(old_mas_representations), move(label_map)));        
    }


    void FactoredTransitionSystem::reinitialize_predecessor_task() {
       
        mas_representations.clear();
        for (size_t index = 0; index < transition_systems.size(); ++index) {
            mas_representations.push_back(
                utils::make_unique_ptr<MergeAndShrinkRepresentationLeaf>
                (index, transition_systems[index]->get_size()));
        }

        label_map.reset(new LabelMap (labels->get_size()));

    }    
    
            
    std::shared_ptr<PlanReconstruction> FactoredTransitionSystem::get_plan_reconstruction() {
        assert(!plan_reconstruction_steps.empty());
        if (plan_reconstruction_steps.size() == 1) {
            return plan_reconstruction_steps[0];
        }
        
        return make_shared<PlanReconstructionSequence>(plan_reconstruction_steps);
    }
    
    std::shared_ptr<task_representation::FTSTask> FactoredTransitionSystem::get_transformed_fts_task() {
        return make_shared<task_representation::FTSTask> (move(transition_systems), move(labels));
    }
}
