#include "task_transformation_merge_and_shrink.h"

#include "factored_transition_system.h"
#include "merge_and_shrink_algorithm.h"
#include "plan_reconstruction.h"

#include "../task_representation/fts_task.h"
#include "../task_representation/labels.h"
#include "../task_representation/sas_task.h"
#include "../task_representation/transition_system.h"

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

    // "Renumber" factors
    int num_factors = fts.get_num_active_entries();
    vector<unique_ptr<task_representation::TransitionSystem>>
        transition_systems;
    transition_systems.reserve(num_factors);
    for (int ts_index : fts) {
        if (fts.is_active(ts_index)) {
            transition_systems.push_back(fts.extract_transition_system(ts_index));
        }
    }

    // TODO: renumber label groups
    unique_ptr<task_representation::Labels> labels = fts.extract_labels();

    shared_ptr<task_representation::FTSTask> fts_task =
        make_shared<task_representation::FTSTask>(move(transition_systems), move(labels));
    shared_ptr<PlanReconstruction> plan_reconstruction = nullptr;
    // TODO: get plan reconstruction from fts and add the final "renumbering"
    // of factors and labels to it.
    return make_pair(fts_task, plan_reconstruction);
}

static shared_ptr<TaskTransformation> _parse(options::OptionParser &parser) {
    add_merge_and_shrink_algorithm_options_to_parser(parser);

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<TaskTransformationMergeAndShrink>(opts);
}

static options::PluginShared<TaskTransformation> _plugin("transform_merge_and_shrink", _parse);
}
