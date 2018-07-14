#ifndef TASK_REPRESENTATION_TASK_TRANSFORMATION_METHOD_H
#define TASK_REPRESENTATION_TASK_TRANSFORMATION_METHOD_H

class TaskTransformationMethod {
public:    
    virtual std::pair<std::shared_ptr<FTSTask>, std::shared_ptr<TaskTransformation>>
	transform_task(const std::shared_ptr<FTSTask> & g_fts_task) = 0;

};

#fi
