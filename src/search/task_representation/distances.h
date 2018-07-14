#ifndef FTS_REPRESENTATIION_DISTANCES_H
#define FTS_REPRESENTATIION_DISTANCES_H

#include "types.h"

#include <vector>

/*
  TODO: Possible interface improvements for this class:
  - Check TODOs in implementation file.

  (Many of these would need performance tests, as distance computation
  can be one of the bottlenecks in our code.)
*/

class TransitionSystem;

class Distances {
    static const int DISTANCE_UNKNOWN = -1;
    
    std::vector<int> distances;
    
    void compute_init_distances_unit_cost(const TransitionSystem &transition_system);
    void compute_goal_distances_unit_cost(const TransitionSystem &transition_system);
    void compute_init_distances_general_cost(const TransitionSystem &transition_system);
    void compute_goal_distances_general_cost(const TransitionSystem &transition_system);
public:
    explicit Distances(const TransitionSystem &transition_system, bool init_distances);
    ~Distances();

    int get_distances(int state) const {
        return distances[state];
    }

    void dump() const;
    void statistics() const;
};


#endif
