#ifndef TASK_TRANSFORMATION_TASK_TRANSFORMATION_METHOD_H
#define TASK_TRANSFORMATION_TASK_TRANSFORMATION_METHOD_H

#include <utility>
#include <memory>

class PlanReconstruction;

namespace task_representation {
    class FTSTask;
}

namespace task_transformation {

    class TaskTransformationMethod {
    public:    
	virtual std::pair<std::shared_ptr<task_representation::FTSTask>,
	    std::shared_ptr<PlanReconstruction>>
	    transform_task(const std::shared_ptr<task_representation::FTSTask> & g_fts_task) = 0;

    };

}
#endif
