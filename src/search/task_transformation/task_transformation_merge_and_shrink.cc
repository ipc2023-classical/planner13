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
        mas_algorithm.build_factored_transition_system(fts_task, false);

    fts.cleanup();
    return make_pair(fts.get_transformed_fts_task(), fts.get_plan_reconstruction());
}
    

    std::pair<std::shared_ptr<task_representation::FTSTask>, 
              std::shared_ptr<StateMapping> >
TaskTransformationMergeAndShrink::transform_task_lossy(
    const std::shared_ptr<task_representation::FTSTask> &fts_task) {
    
    MergeAndShrinkAlgorithm mas_algorithm(options);
    
    FactoredTransitionSystem fts =
        mas_algorithm.build_factored_transition_system(fts_task, true);
    
    fts.cleanup();

    return make_pair(fts.get_transformed_fts_task(), fts.get_state_mapping());

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
