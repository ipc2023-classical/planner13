#ifndef SYMBOLIC_OPT_ORDER_H
#define SYMBOLIC_OPT_ORDER_H

#include "../utils/rng.h"
#include <vector>
#include <memory>
#include "../task_representation/fts_task.h"

namespace symbolic {
    class InfluenceGraph {
        std::vector<std::vector<double>> influence_graph;

        double influence(int v1, int v2) const {
            return influence_graph[v1][v2];
        }


        double optimize_variable_ordering_gamer(std::vector<int> &order, int iterations,
                                                const std::shared_ptr<utils::RandomNumberGenerator> &rng) const;

        double compute_function(const std::vector<int> &order) const;

        void optimize_ordering_gamer(std::vector<int> &ordering) const;

        static void randomize(std::vector<int> &ordering, std::vector<int> &new_order,
                              const std::shared_ptr<utils::RandomNumberGenerator> &rng);


    public:
        InfluenceGraph(int n);

        void get_ordering(std::vector<int> &ordering, const std::shared_ptr<utils::RandomNumberGenerator> &rng) const;

        void optimize_variable_ordering_gamer(std::vector<int> &order,
                                              std::vector<int> &partition_begin,
                                              std::vector<int> &partition_sizes,
                                              const std::shared_ptr<utils::RandomNumberGenerator> &rng,
                                              int iterations = 50000) const;

        void set_influence(int v1, int v2, double val = 1) {
            influence_graph[v1][v2] = val;
            influence_graph[v2][v1] = val;
        }

        static void compute_gamer_ordering(std::vector<int> &ordering,
                                           const std::shared_ptr<task_representation::FTSTask> &task,
                                           const std::shared_ptr<utils::RandomNumberGenerator> &rng
        );
    };
}


#endif
