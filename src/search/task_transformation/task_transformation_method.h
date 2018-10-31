#ifndef TASK_TRANSFORMATION_TASK_TRANSFORMATION_METHOD_H
#define TASK_TRANSFORMATION_TASK_TRANSFORMATION_METHOD_H

#include <utility>
#include <memory>

namespace task_representation {
    class FTSTask;
    class SASTask;
}

namespace task_transformation {
class PlanReconstruction;

class TaskTransformationMethod {
public:
    virtual ~TaskTransformationMethod() = default;
    virtual std::pair<std::shared_ptr<task_representation::FTSTask>,
        std::shared_ptr<PlanReconstruction>> transform_task(
            const task_representation::SASTask &sas_task) = 0;
};
}
#endif
