#include "task_transformation_method.h"

#include "../options/plugin.h"

using namespace std;

namespace task_transformation {
static options::PluginTypePlugin<TaskTransformationMethod> _type_plugin(
    "TaskTransformationMethod",
    "This page describes the various task transformation methods s supported "
    "by the planner.");
}
