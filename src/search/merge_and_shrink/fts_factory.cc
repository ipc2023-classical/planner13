#include "fts_factory.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "label_equivalence_relation.h"
#include "labels.h"
#include "merge_and_shrink_representation.h"
#include "transition_system.h"
#include "types.h"

#include "../task_representation/fts_task.h"
#include "../task_representation/label_equivalence_relation.h"
#include "../task_representation/labels.h"
#include "../task_representation/transition_system.h"

#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <vector>

using namespace std;

namespace merge_and_shrink {
class FTSFactory {
    const task_representation::FTSTask &fts_task;

    // see TODO in build_transitions()
    int task_has_conditional_effects;

    vector<unique_ptr<Label>> create_labels();
    unique_ptr<TransitionSystem> copy_ts(
        const task_representation::TransitionSystem &ts, const Labels &labels) const;
    vector<unique_ptr<TransitionSystem>> create_transition_systems(
        const Labels &labels) const;
    vector<unique_ptr<MergeAndShrinkRepresentation>> create_mas_representations() const;
    vector<unique_ptr<Distances>> create_distances(
        const vector<unique_ptr<TransitionSystem>> &transition_systems) const;
public:
    explicit FTSFactory(const task_representation::FTSTask &fts_task);
    ~FTSFactory();

    /*
      Note: create() may only be called once. We don't worry about
      misuse because the class is only used internally in this file.
    */
    FactoredTransitionSystem create(
        bool compute_init_distances,
        bool compute_goal_distances,
        Verbosity verbosity);
};


FTSFactory::FTSFactory(const task_representation::FTSTask &fts_task)
    : fts_task(fts_task), task_has_conditional_effects(false) {
}

FTSFactory::~FTSFactory() {
}

vector<unique_ptr<Label>> FTSFactory::create_labels() {
    const task_representation::Labels &labels = fts_task.get_labels();
    assert(labels.get_num_active_entries() == labels.get_size());
    vector<unique_ptr<Label>> result;
    for (int label_no = 0; label_no < labels.get_size(); ++label_no) {
        assert(labels.is_current_label(label_no));
        result.push_back(utils::make_unique_ptr<Label>(labels.get_label_cost(label_no)));
    }
    return result;
}

unique_ptr<TransitionSystem> FTSFactory::copy_ts(
    const task_representation::TransitionSystem &ts, const Labels &labels) const {
    int num_variables = fts_task.get_size();
    vector<int> incorporated_variables(ts.get_incorporated_variables());

    std::unique_ptr<LabelEquivalenceRelation> label_equivalence_relation =
        utils::make_unique_ptr<LabelEquivalenceRelation>(labels);
    std::vector<std::vector<Transition>> transitions_by_group_id(labels.get_max_size());
    for (const task_representation::GroupAndTransitions &gat : ts) {
        const task_representation::LabelGroup &label_group = gat.label_group;
        int group_id = label_equivalence_relation->add_label_group(label_group.begin(), label_group.end());
        const vector<task_representation::Transition> &transitions = gat.transitions;
        vector<Transition> transitions_copy;
        transitions_copy.reserve(transitions.size());
        for (const task_representation::Transition &transition : transitions) {
            transitions_copy.emplace_back(transition.src, transition.target);
        }
        transitions_by_group_id[group_id] = transitions_copy;
    }

    int num_states = ts.get_size();
    vector<bool> goal_states(ts.get_is_goal());
    int init_state = ts.get_init_state();
    const bool compute_label_equivalence_relation = false;
    return utils::make_unique_ptr<TransitionSystem>(
        num_variables,
        move(incorporated_variables),
        move(label_equivalence_relation),
        move(transitions_by_group_id),
        num_states,
        move(goal_states),
        init_state,
        compute_label_equivalence_relation);
}

vector<unique_ptr<TransitionSystem>> FTSFactory::create_transition_systems(
    const Labels &labels) const {
    // Create the actual TransitionSystem objects.
    int num_variables = fts_task.get_size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<TransitionSystem>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        const task_representation::TransitionSystem &ts = fts_task.get_ts(var_no);
        result.push_back(copy_ts(ts, labels));
    }
    return result;
}

vector<unique_ptr<MergeAndShrinkRepresentation>> FTSFactory::create_mas_representations() const {
    // Create the actual MergeAndShrinkRepresentation objects.
    int num_variables = fts_task.get_size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<MergeAndShrinkRepresentation>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        int range = fts_task.get_ts(var_no).get_size();
        result.push_back(
            utils::make_unique_ptr<MergeAndShrinkRepresentationLeaf>(var_no, range));
    }
    return result;
}

vector<unique_ptr<Distances>> FTSFactory::create_distances(
    const vector<unique_ptr<TransitionSystem>> &transition_systems) const {
    // Create the actual Distances objects.
    int num_variables = fts_task.get_size();

    // We reserve space for the transition systems added later by merging.
    vector<unique_ptr<Distances>> result;
    assert(num_variables >= 1);
    result.reserve(num_variables * 2 - 1);

    for (int var_no = 0; var_no < num_variables; ++var_no) {
        result.push_back(
            utils::make_unique_ptr<Distances>(*transition_systems[var_no]));
    }
    return result;
}

FactoredTransitionSystem FTSFactory::create(
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity) {
    if (verbosity >= Verbosity::NORMAL) {
        cout << "Building atomic transition systems... " << endl;
    }

    unique_ptr<Labels> labels = utils::make_unique_ptr<Labels>(create_labels());

    vector<unique_ptr<TransitionSystem>> transition_systems =
        create_transition_systems(*labels);
    vector<unique_ptr<MergeAndShrinkRepresentation>> mas_representations =
        create_mas_representations();
    vector<unique_ptr<Distances>> distances =
        create_distances(transition_systems);

    return FactoredTransitionSystem(
        move(labels),
        move(transition_systems),
        move(mas_representations),
        move(distances),
        compute_init_distances,
        compute_goal_distances,
        verbosity);
}

FactoredTransitionSystem create_factored_transition_system(
    const task_representation::FTSTask &fts_task,
    const bool compute_init_distances,
    const bool compute_goal_distances,
    Verbosity verbosity) {
    return FTSFactory(fts_task).create(
        compute_init_distances,
        compute_goal_distances,
        verbosity);
}
}
