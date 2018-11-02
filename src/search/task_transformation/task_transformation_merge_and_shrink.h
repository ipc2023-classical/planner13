#ifndef TASK_TRANSFORMATION_TASK_TRANSFORMATION_MERGE_AND_SHRINK_H
#define TASK_TRANSFORMATION_TASK_TRANSFORMATION_MERGE_AND_SHRINK_H

#include "task_transformation.h"

#include "../option_parser.h"

namespace task_transformation {
class TaskTransformationMergeAndShrink : public TaskTransformation {
    options::Options options;
public:
    explicit TaskTransformationMergeAndShrink(const options::Options &options);
    virtual ~TaskTransformationMergeAndShrink() = default;
    virtual std::pair<std::shared_ptr<task_representation::FTSTask>,
        std::shared_ptr<PlanReconstruction>> transform_task(
            const std::shared_ptr<task_representation::FTSTask> &fts_task) override;
};
}
#endif
