#ifndef TASK_TRANSFORMATION_TASK_TRANSFORMATION_H
#define TASK_TRANSFORMATION_TASK_TRANSFORMATION_H

#include <utility>
#include <memory>

#include "../option_parser.h"

namespace task_representation {
    class FTSTask;
    class SASTask;
}

namespace task_transformation {
class PlanReconstruction;
class StateMapping;

class TaskTransformation {
public:
    virtual ~TaskTransformation() = default;
    virtual std::pair<std::shared_ptr<task_representation::FTSTask>,
        std::shared_ptr<PlanReconstruction>> transform_task(
            const std::shared_ptr<task_representation::FTSTask> &fts_task) = 0;


    //We do not care about plan reconstruction 
    virtual std::pair<std::shared_ptr<task_representation::FTSTask>,
        std::shared_ptr<StateMapping> > transform_task_lossy(
            const std::shared_ptr<task_representation::FTSTask> &fts_task) = 0;

};

class NoTransformation : public TaskTransformation {
public:
    virtual ~NoTransformation() = default;
    virtual std::pair<std::shared_ptr<task_representation::FTSTask>,
        std::shared_ptr<PlanReconstruction>> transform_task(
            const std::shared_ptr<task_representation::FTSTask> &fts_task)  override;

    //We do not care about plan reconstruction 
    virtual std::pair<std::shared_ptr<task_representation::FTSTask>, 
        std::shared_ptr<StateMapping> > transform_task_lossy(
            const std::shared_ptr<task_representation::FTSTask> &fts_task)  override;

};
}
#endif
