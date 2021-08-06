#ifndef FAST_DOWNWARD_STATE_REORDERING_H
#define FAST_DOWNWARD_STATE_REORDERING_H

#include "sym_bucket.h"
#include "../utils/timer.h"
#include "../globals.h"
#include "../task_representation/fts_task.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <map>
#include <cassert>

namespace options {
    class Options;

    class OptionParser;
}
namespace symbolic {
    class StateReordering {
    private:
    public:
        std::string name = "abstract";

        virtual void computeStateReordering(std::vector<int>& var_order, std::map<int, std::vector<int>>& var_to_state, const std::shared_ptr<task_representation::FTSTask>& _task) = 0;
    };

    class DefaultStateReordering : public StateReordering {
    private:
    public:
        explicit DefaultStateReordering(const options::Options&);
        virtual ~DefaultStateReordering() = default;

        void computeStateReordering(std::vector<int>& var_order, std::map<int,
                                    std::vector<int>>& var_to_state,
                                    const std::shared_ptr<task_representation::FTSTask>& _task) override;
    };

    class RandomStateReordering : public StateReordering {
    private:
        int random_seed; // only for dump options
        std::shared_ptr<utils::RandomNumberGenerator> rng;
    public:
        explicit RandomStateReordering(const options::Options&);
        void computeStateReordering(std::vector<int>& var_order,
                                    std::map<int, std::vector<int>>& var_to_state,
                                    const std::shared_ptr<task_representation::FTSTask>& _task) override;
        virtual ~RandomStateReordering() = default;
    };
}
#endif //FAST_DOWNWARD_STATE_REORDERING_H
