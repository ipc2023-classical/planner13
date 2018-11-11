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

    auto mapping = fts.cleanup();
    auto mas_representations = move(mapping.first);
    auto label_map = move(mapping.second);
    auto transition_systems = fts.extract_transition_systems();
    auto labels = fts.extract_labels(); 

    cout << "Collection information on plan reconstruction..." << endl;
    shared_ptr<task_representation::FTSTask> transformed_fts_task =
        make_shared<task_representation::FTSTask>(move(transition_systems), move(labels));
    
    shared_ptr<PlanReconstructionMergeAndShrink> plan_reconstruction =
        make_shared<PlanReconstructionMergeAndShrink>(
            fts_task, move(mas_representations), move(label_map));
    
    return make_pair(transformed_fts_task, plan_reconstruction);
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
