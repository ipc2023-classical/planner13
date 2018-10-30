#ifndef TASK_REPRESENTATION_STATE_H
#define TASK_REPRESENTATION_STATE_H

#include "fts_task.h"
#include "fts_operators.h"
#include "fact.h"

#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/system.h"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

namespace task_representation {
class State {
    const FTSTask *task;
    std::vector<int> values;
public:
    State(const FTSTask &task, std::vector<int> &&values)
        : task(&task), values(std::move(values)) {
        assert(static_cast<int>(size()) == this->task->get_size());
    }
    ~State() = default;
    State(const State &) = default;

    State(State &&other)
        : task(other.task), values(std::move(other.values)) {
        other.task = nullptr;
    }

    State &operator=(const State &&other) {
        if (this != &other) {
            values = std::move(other.values);
        }
        return *this;
    }

    bool operator==(const State &other) const {
        assert(task == other.task);
        return values == other.values;
    }

    bool operator!=(const State &other) const {
        return !(*this == other);
    }

    std::size_t hash() const {
        std::hash<std::vector<int>> hasher;
        return hasher(values);
    }

    std::size_t size() const {
        return values.size();
    }

    /* Fact operator[](std::size_t var_id) const { */
    /*     assert(var_id < size()); */
    /*     return Fact(*task, var_id, values[var_id]); */
    /* } */

    /* Fact operator[](int var) const { */
    /*     return (*this)[var]; */
    /* } */

    int operator[](std::size_t var_id) const {
        return values[var_id];
    }

    int operator[](int var) const {
        return (*this)[var];
    }

    inline FTSTask get_task() const;

    const std::vector<int> &get_values() const {
        return values;
    }

    /* State get_successor(const FTSOperator & op) const; */

    void dump_pddl() const;
    void dump_fdr() const;

};
}

#endif
