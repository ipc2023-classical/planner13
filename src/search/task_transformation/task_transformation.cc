#include "task_transformation.h"

#include "../options/plugin.h"
#include "../plugin.h"
#include "plan_reconstruction.h"

using namespace std;

namespace task_transformation {


std::pair<std::shared_ptr<task_representation::FTSTask>,
          std::shared_ptr<PlanReconstruction>>
NoTransformation::transform_task(const std::shared_ptr<task_representation::FTSTask> &fts_task) {

    
    return make_pair(std::shared_ptr<task_representation::FTSTask>(fts_task),
                      std::make_shared<PlanReconstructionSequence>(
                          vector<shared_ptr<PlanReconstruction>>()));

}

    //We do not care about plan reconstruction 
 std::pair<std::shared_ptr<task_representation::FTSTask>, Mapping >
 NoTransformation::transform_task_lossy(
    const std::shared_ptr<task_representation::FTSTask> &fts_task)   {
    return make_pair(
        std::shared_ptr<task_representation::FTSTask>(fts_task),
        Mapping());
}

static options::PluginTypePlugin<TaskTransformation> _type_plugin(
    "TaskTransformation",
    "This page describes the various task transformation methods s supported "
    "by the planner.");



    static shared_ptr<TaskTransformation> _parse(options::OptionParser &parser) {
    options::Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
        return make_shared<NoTransformation>();
    }
}

static options::PluginShared<TaskTransformation> _plugin("none", _parse);

}
