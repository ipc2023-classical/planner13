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
    std::vector<int> values; // We represent dead end states by an empty vector of values.
public:
State(const FTSTask &task, std::vector<int> &&values)
    : task(&task), values(std::move(values)) {
        assert(is_dead_end() || static_cast<int>(size()) == this->task->get_size());
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

    bool is_dead_end() const {
        return values.empty();
    }
    std::size_t size() const {
        assert(!is_dead_end());
        return values.size();
    }

    int operator[](std::size_t var_id) const {
        assert(!is_dead_end());
        return values[var_id];
    }

    int operator[](int var) const {
        assert(!is_dead_end());        
        return values[var];
    }

    inline FTSTask get_task() const;

    const std::vector<int> &get_values() const {
        assert(!is_dead_end());
        return values;
    }

    /* State get_successor(const FTSOperator & op) const; */

    void dump_pddl() const;
    void dump_fdr() const;

};
}

#endif
