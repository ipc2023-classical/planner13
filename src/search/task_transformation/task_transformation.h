#ifndef TASK_TRANSFORMATION_TASK_TRANSFORMATION_H
#define TASK_TRANSFORMATION_TASK_TRANSFORMATION_H

#include <utility>
#include <memory>

namespace task_representation {
    class FTSTask;
    class SASTask;
}

namespace task_transformation {
class PlanReconstruction;

class TaskTransformation {
public:
    virtual ~TaskTransformation() = default;
    virtual std::pair<std::shared_ptr<task_representation::FTSTask>,
        std::shared_ptr<PlanReconstruction>> transform_task(
            const task_representation::SASTask &sas_task) = 0;
};
}
#endif
