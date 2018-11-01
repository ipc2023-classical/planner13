#include "task_transformation.h"

#include "../options/plugin.h"

using namespace std;

namespace task_transformation {
static options::PluginTypePlugin<TaskTransformation> _type_plugin(
    "TaskTransformation",
    "This page describes the various task transformation methods s supported "
    "by the planner.");
}
