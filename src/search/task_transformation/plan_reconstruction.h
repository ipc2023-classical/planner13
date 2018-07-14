



class PlanReconstruction {
    // Given a sequence of actions that is a plan for the successor task, retrieve a plan
    // for the predecessor task
    virtual void reconstruct_plan(std::vector<OperatorID> & plan) const;   
};

class LabelReductionPlanReconstruction : public PlanReconstruction {
    std::shared_ptr<Task> predecessor_task;
    std::shared_ptr<Operators> predecessor_ops, successor_ops;
    LabelMap label_map;

    virtual void reconstruct_plan(std::vector<OperatorID> & plan) const;
};


class ShrinkingPlanReconstruction : public PlanReconstruction {
    std::shared_ptr<Task> predecessor_task;
    std::shared_ptr<Operators> predecessor_ops, successor_ops;
    LabelMap label_map;

    virtual void transform_plan(std::vector<OperatorID> & plan) const {
	return plan;
    }
};

