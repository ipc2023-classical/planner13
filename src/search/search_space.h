#ifndef SEARCH_SPACE_H
#define SEARCH_SPACE_H

#include "global_state.h"
#include "operator_cost.h"
#include "per_state_information.h"
#include "search_node_info.h"

#include <vector>

class GlobalState;
class Plan;

class SearchNode {
    const StateRegistry &state_registry;
    StateID state_id;
    SearchNodeInfo &info;
    OperatorCost cost_type;
public:
    SearchNode(const StateRegistry &state_registry,
               StateID state_id,
               SearchNodeInfo &info,
               OperatorCost cost_type);

    StateID get_state_id() const {
        return state_id;
    }
    GlobalState get_state() const;

    bool is_new() const;
    bool is_open() const;
    bool is_closed() const;
    bool is_dead_end() const;

    int get_g() const;
    int get_real_g() const;

    void open_initial();
    void open(const SearchNode &parent_node, OperatorID creating_operator, int cost);
    void reopen(const SearchNode &parent_node, OperatorID creating_operator, int cost);
    void update_parent(const SearchNode &parent_node, OperatorID creating_operator, int cost);
    void close();
    void mark_as_dead_end();

    void dump() const;
};


class SearchSpace {
    PerStateInformation<SearchNodeInfo> search_node_infos;

    StateRegistry &state_registry;
    OperatorCost cost_type;
public:
    SearchSpace(StateRegistry &state_registry, OperatorCost cost_type);

    SearchNode get_node(const GlobalState &state);
    void trace_path(const GlobalState &goal_state, Plan &path) const;
    void set_path(const PlanState &state, Plan& _plan, const std::vector<PlanState> &states, const std::vector<OperatorID> &ops,
                  const std::shared_ptr<task_representation::FTSTask> &sharedPtr) const;

    void dump() const;

    void print_statistics() const;
};

#endif
