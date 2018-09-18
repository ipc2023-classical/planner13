#ifndef TASK_REPRESENTATION_FACT_H
#define TASK_REPRESENTATION_FACT_H


#include "../utils/hash.h"
#include "../utils/collections.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <cassert>

namespace task_representation {
class FTSTask;

struct FactPair {
    int var;
    int value;

    FactPair(int var, int value)
        : var(var), value(value) {
    }

    bool operator<(const FactPair &other) const {
        return var < other.var || (var == other.var && value < other.value);
    }

    bool operator==(const FactPair &other) const {
        return var == other.var && value == other.value;
    }

    bool operator!=(const FactPair &other) const {
        return var != other.var || value != other.value;
    }

    /*
      This special object represents "no such fact". E.g., functions
      that search a fact can return "no_fact" when no matching fact is
      found.
    */
    static const FactPair no_fact;
};
}

std::ostream &operator<<(std::ostream &os, const task_representation::FactPair &fact_pair);

namespace std {
template<>
struct hash<task_representation::FactPair> {
    size_t operator()(const task_representation::FactPair &fact) const {
        std::pair<int, int> raw_fact(fact.var, fact.value);
        std::hash<std::pair<int, int>> hasher;
        return hasher(raw_fact);
    }
};
}

namespace task_representation {
class Fact {
    const FTSTask *task;
    FactPair fact;
public:
    Fact(const FTSTask &task, int var_id, int value);
    Fact(const FTSTask &task, const FactPair &fact);
    ~Fact() = default;

    /* VariableProxy get_variable() const; */

    int get_value() const {
        return fact.value;
    }

    FactPair get_pair() const {
        return fact;
    }

    std::string get_name() const;

    bool operator==(const Fact &other) const {
        assert(task == other.task);
        return fact == other.fact;
    }

    bool operator!=(const Fact &other) const {
        return !(*this == other);
    }

    bool is_mutex(const Fact &other) const;
};
}

#endif
