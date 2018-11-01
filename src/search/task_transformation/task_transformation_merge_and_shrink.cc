#include "task_transformation_merge_and_shrink.h"

#include "factored_transition_system.h"
#include "merge_and_shrink_algorithm.h"
#include "plan_reconstruction.h"

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
        const task_representation::SASTask &sas_task) {
    MergeAndShrinkAlgorithm mas_algorithm(options);
    FactoredTransitionSystem fts =
        mas_algorithm.build_factored_transition_system(sas_task);

    // "Renumber" factors consecutively. (Actually, nothing to do except
    // storing them consecutively since factor indices are not stored anywhere.)
    int num_factors = fts.get_num_active_entries();
    cout << "Number of remaining factors: " << num_factors << endl;
    vector<unique_ptr<task_representation::TransitionSystem>>
        transition_systems;
    transition_systems.reserve(num_factors);
    for (int ts_index : fts) {
        if (fts.is_active(ts_index)) {
            transition_systems.push_back(fts.extract_transition_system(ts_index));
        }
    }
    cout << "Done renumbering factors." << endl;

    // Renumber labels consecutively
    unique_ptr<Labels> labels = fts.extract_labels();
    // TODO: extract label map and update it
    int num_labels = labels->get_num_active_entries();
    cout << "Number of remaining labels: " << num_labels << endl;
    vector<unique_ptr<task_representation::Label>> active_labels;
    vector<pair<int, int>> label_mapping;
    active_labels.reserve(num_labels);
    label_mapping.reserve(num_labels);
    for (int label_no = 0; label_no < labels->get_size(); ++label_no) {
        if (labels->is_current_label(label_no)) {
            active_labels.push_back(labels->extract_label(label_no));
            int new_label_no = label_mapping.size();
            label_mapping.emplace_back(label_no, new_label_no);
        }
    }
    cout << "Renumbering labels: " << label_mapping << endl;
    for (unique_ptr<TransitionSystem> &ts : transition_systems) {
        ts->renumber_labels(label_mapping);
    }
    cout << "Done renumbering labels." << endl;

    cout << "Collection information on plan reconstruction..." << endl;
    shared_ptr<task_representation::FTSTask> fts_task =
        make_shared<task_representation::FTSTask>(move(transition_systems), move(active_labels));
    shared_ptr<PlanReconstruction> plan_reconstruction = nullptr;
    // TODO: get plan reconstruction from fts and add the final "renumbering"
    // of factors and labels to it.
    return make_pair(fts_task, plan_reconstruction);
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
