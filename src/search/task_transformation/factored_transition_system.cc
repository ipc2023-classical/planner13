#include "factored_transition_system.h"

#include "distances.h"
#include "merge_and_shrink_representation.h"
#include "utils.h"

#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"
#include "../task_representation/label_equivalence_relation.h"

#include "../utils/collections.h"
#include "../utils/memory.h"
#include "../utils/system.h"

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
    unique_ptr<Labels> labels,
    vector<unique_ptr<TransitionSystem>> &&transition_systems,
    vector<unique_ptr<MergeAndShrinkRepresentation>> &&mas_representations,
    vector<unique_ptr<Distances>> &&distances,
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity)
    : labels(move(labels)),
      transition_systems(move(transition_systems)),
      mas_representations(move(mas_representations)),
      distances(move(distances)),
      compute_init_distances(compute_init_distances),
      compute_goal_distances(compute_goal_distances),
      num_active_entries(this->transition_systems.size()) {
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
    Verbosity verbosity) {
    assert(is_component_valid(index));

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
    mas_representations[index]->apply_abstraction_to_lookup_table(
        abstraction_mapping);

    /* If distances need to be recomputed, this already happened in the
       Distances object. */
    assert(is_component_valid(index));
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
    for (const auto &new_label_old_labels : label_mapping) {
        assert(new_label_old_labels.first == labels->get_size());
        labels->reduce_labels(new_label_old_labels.second);
    }
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

unique_ptr<TransitionSystem> FactoredTransitionSystem::extract_transition_system(int index) {
    // This assertion does not hold since we don't extract both transition
    // system and merge-and-shrink representation at the same time.
//    assert(is_active(index));
    return move(transition_systems[index]);
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
  

bool FactoredTransitionSystem::remove_irrelevant_transition_systems() {
    bool removed_tr = false;
    for (size_t index = 0; index < transition_systems.size(); ++index) {
        if (transition_systems[index] && transition_systems[index]->get_size() <= 1) {
            transition_systems[index] = nullptr;
            distances[index] = nullptr;
            mas_representations[index] = nullptr;
            --num_active_entries;
            removed_tr = true;
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
        if (labels->is_current_label(label_no) && is_irrelevant_label(LabelID(label_no))) {
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


    

}
