#include "task_transformation_merge_and_shrink.h"

#include "factored_transition_system.h"
#include "label_map.h"
#include "merge_and_shrink_algorithm.h"
#include "merge_and_shrink_representation.h"
#include "plan_reconstruction_merge_and_shrink.h"

#include "../task_representation/fts_task.h"
#include "../task_representation/labels.h"
#include "../task_representation/sas_task.h"
#include "../task_representation/transition_system.h"

#include "../utils/logging.h"

#include "../plugin.h"

using namespace std;

namespace task_transformation {
TaskTransformationMergeAndShrink::TaskTransformationMergeAndShrink(
    const Options &options) : TaskTransformation(), options(options) {
}

pair<shared_ptr<task_representation::FTSTask>, shared_ptr<PlanReconstruction>>
    TaskTransformationMergeAndShrink::transform_task(
        const shared_ptr<task_representation::FTSTask> &fts_task) {
    MergeAndShrinkAlgorithm mas_algorithm(options);
    FactoredTransitionSystem fts =
        mas_algorithm.build_factored_transition_system(*fts_task);

    // "Renumber" factors consecutively. (Actually, nothing to do except
    // storing them consecutively since factor indices are not stored anywhere.)
    int num_factors = fts.get_num_active_entries();
    cout << "Number of remaining factors: " << num_factors << endl;
    vector<unique_ptr<task_representation::TransitionSystem>>
        transition_systems;
    vector<unique_ptr<MergeAndShrinkRepresentation>> mas_representations;
    transition_systems.reserve(num_factors);
    mas_representations.reserve(num_factors);
    for (int ts_index : fts) {
        if (fts.is_active(ts_index)) {
            transition_systems.push_back(fts.extract_transition_system(ts_index));
            mas_representations.push_back(fts.extract_mas_representation(ts_index));
        }
    }
    cout << "Done renumbering factors." << endl;

    // Renumber labels consecutively
    unique_ptr<Labels> labels = fts.extract_labels();
    int new_num_labels = labels->get_num_active_entries();
    cout << "Number of remaining labels: " << new_num_labels << endl;
    vector<unique_ptr<task_representation::Label>> active_labels;
    vector<int> old_to_new_labels(labels->get_size(), -1);
    active_labels.reserve(new_num_labels);
    for (int label_no = 0; label_no < labels->get_size(); ++label_no) {
        if (labels->is_current_label(label_no)) {
            int new_label_no = active_labels.size();
            active_labels.push_back(labels->extract_label(label_no));
            old_to_new_labels[label_no] = new_label_no;
        }
    }
    cout << "Renumbering labels: " << old_to_new_labels << endl;
    for (unique_ptr<TransitionSystem> &ts : transition_systems) {
        ts->renumber_labels(old_to_new_labels, new_num_labels);
    }
    unique_ptr<LabelMap> label_map = mas_algorithm.extract_label_map();
    label_map->update(old_to_new_labels);
    // TODO: if we every wanted to continue using new_labels and apply further
    // label reductions, we should pass a larger number as the "max_size" than
    // the number of current labels.
    unique_ptr<Labels> new_labels = utils::make_unique_ptr<Labels>(
        move(active_labels), active_labels.size());
    cout << "Done renumbering labels." << endl;

    cout << "Collection information on plan reconstruction..." << endl;
    shared_ptr<task_representation::FTSTask> transformed_fts_task =
        make_shared<task_representation::FTSTask>(
            move(transition_systems), move(new_labels));
    shared_ptr<PlanReconstructionMergeAndShrink> plan_reconstruction =
        make_shared<PlanReconstructionMergeAndShrink>(
            fts_task, move(mas_representations), move(label_map));
    return make_pair(transformed_fts_task, plan_reconstruction);
}

static shared_ptr<TaskTransformation> _parse(options::OptionParser &parser) {
    add_merge_and_shrink_algorithm_options_to_parser(parser);
    options::Options opts = parser.parse();
    handle_shrink_limit_options_defaults(opts);
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<TaskTransformationMergeAndShrink>(opts);
}

static options::PluginShared<TaskTransformation> _plugin("transform_merge_and_shrink", _parse);
}
