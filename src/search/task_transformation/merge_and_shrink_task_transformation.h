#ifndef TASK_TRANSFORMATION_MERGE_AND_SHRINK_TASK_TRANSFORMATION_H
#define TASK_TRANSFORMATION_MERGE_AND_SHRINK_TASK_TRANSFORMATION_H

#include "task_transformation_method.h"

#include "../option_parser.h"

namespace task_transformation {
class MergeAndShrinkTaskTransformation : public TaskTransformationMethod {
    options::Options options;
public:
    explicit MergeAndShrinkTaskTransformation(const options::Options &options);
    virtual ~MergeAndShrinkTaskTransformation() = default;
    virtual std::pair<std::shared_ptr<task_representation::FTSTask>,
        std::shared_ptr<PlanReconstruction>> transform_task(
            const task_representation::SASTask &sas_task) override;
};
}
#endif
