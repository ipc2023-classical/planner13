#ifndef FTS_REPRESENTATION_TYPES_H
#define FTS_REPRESENTATION_TYPES_H

#include <forward_list>
#include <vector>

// Positive infinity. The name "INFINITY" is taken by an ISO C99 macro.
extern const int INF;
extern const int MINUSINF;
extern const int PRUNED_STATE;

/*
  An equivalence class is a set of abstract states that shall be
  mapped (shrunk) to the same abstract state.

  An equivalence relation is a partitioning of states into
  equivalence classes. It may omit certain states entirely; these
  will be dropped completely and receive an h value of infinity.
*/
using StateEquivalenceClass = std::forward_list<int>;
using StateEquivalenceRelation = std::vector<StateEquivalenceClass>;

enum class Verbosity {
    SILENT,
    NORMAL,
    VERBOSE
};

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

namespace std {
    template<> struct hash<LabelGroupID> {
    public:
	size_t operator()(const LabelGroupID & g) const 
	{
	    return g.id;
	}
    };

    template<> struct hash<LabelID> {
    public:
	size_t operator()(const LabelID & g) const 
	{
	    return g.id;
	}
    };
}



#endif
