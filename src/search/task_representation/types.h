#ifndef TASK_REPRESENTATION_TYPES_H
#define TASK_REPRESENTATION_TYPES_H

#include <functional>
#include <list>

namespace task_representation {
// Related to representation of grouped labels
using LabelIter = std::list<int>::iterator;
using LabelConstIter = std::list<int>::const_iterator;

struct LabelID {
    int id;
    LabelID() : id(0) {
    }
    explicit LabelID(int id_) :
    id(id_) {
    }
    operator int() const {
        return id;
    }

    LabelID & operator++ () {
        ++id;
        return *this;
    }
};

struct LabelGroupID {
    public:
    int id;
    LabelGroupID() : id(0) {
    }
    explicit LabelGroupID(int id_) :
    id(id_) {
    }
    operator int() const {
        return id;
    }

    LabelGroupID & operator++ () {
        ++id;
        return *this;
    }
};
}

namespace std {
    template<> struct hash<task_representation::LabelGroupID> {
    public:
    size_t operator()(const task_representation::LabelGroupID & g) const
        {
            return g.id;
        }
    };
}

#endif
