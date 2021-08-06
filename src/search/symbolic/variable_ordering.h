#ifndef FAST_DOWNWARD_VARIABLE_ORDERING_H
#define FAST_DOWNWARD_VARIABLE_ORDERING_H

#include "../task_representation/fts_task.h"
#include "../utils/rng.h"
#include <vector>

namespace options {
    class Options;

    class OptionParser;
}
namespace symbolic {
    class VariableOrdering {
    private:
    public:
        std::string name = "abstract";

        virtual void compute_variable_ordering(std::vector<int> &var_order, const std::shared_ptr<task_representation::FTSTask>& task) = 0;
    };

    class InputVariableOrdering : public VariableOrdering {
    private:
    public:
        explicit InputVariableOrdering(options::Options&);
        virtual ~InputVariableOrdering() = default;
        void compute_variable_ordering(std::vector<int> &var_order, const std::shared_ptr<task_representation::FTSTask>& task) override;
    };

    class GamerVariableOrdering : public VariableOrdering {
    private:
        int random_seed; // only for dump options
        std::shared_ptr<utils::RandomNumberGenerator> rng;
    public:
        explicit GamerVariableOrdering(options::Options&);
        virtual ~GamerVariableOrdering() = default;
        void compute_variable_ordering(std::vector<int> &var_order, const std::shared_ptr<task_representation::FTSTask>& task) override;
    };
}
#endif //FAST_DOWNWARD_VARIABLE_ORDERING_H
