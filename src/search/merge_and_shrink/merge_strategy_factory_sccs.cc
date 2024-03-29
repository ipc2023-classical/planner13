#include "merge_strategy_factory_sccs.h"

#include "merge_strategy_sccs.h"
#include "merge_selector.h"
#include "merge_tree_factory.h"
#include "transition_system.h"

#include "../task_representation/fts_task.h"

#include "../algorithms/sccs.h"
#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"
#include "../task_utils/causal_graph.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
bool compare_sccs_increasing(const vector<int> &lhs, const vector<int> &rhs) {
    return lhs.size() < rhs.size();
}

bool compare_sccs_decreasing(const vector<int> &lhs, const vector<int> &rhs) {
    return lhs.size() > rhs.size();
}

MergeStrategyFactorySCCs::MergeStrategyFactorySCCs(const options::Options &options)
    : order_of_sccs(static_cast<OrderOfSCCs>(options.get_enum("order_of_sccs"))),
      merge_tree_factory(nullptr),
      merge_selector(nullptr) {
    if (options.contains("merge_tree")) {
        merge_tree_factory = options.get<shared_ptr<MergeTreeFactory>>("merge_tree");
    }
    if (options.contains("merge_selector")) {
        merge_selector = options.get<shared_ptr<MergeSelector>>("merge_selector");
    }
}

unique_ptr<MergeStrategy> MergeStrategyFactorySCCs::compute_merge_strategy(
    const task_representation::FTSTask &fts_task,
    const FactoredTransitionSystem &fts) {
    VariablesProxy vars = fts_task.get_variables();
    int num_vars = vars.size();

    // Compute SCCs of the causal graph.
    vector<vector<int>> cg;
    cg.reserve(num_vars);
    for (VariableProxy var : vars) {
        const vector<int> &successors =
            fts_task.get_causal_graph().get_successors(var.get_id());
        cg.push_back(successors);
    }
    vector<vector<int>> sccs(sccs::compute_maximal_sccs(cg));

    // Put the SCCs in the desired order.
    switch (order_of_sccs) {
    case OrderOfSCCs::TOPOLOGICAL:
        // SCCs are computed in topological order.
        break;
    case OrderOfSCCs::REVERSE_TOPOLOGICAL:
        // SCCs are computed in topological order.
        reverse(sccs.begin(), sccs.end());
        break;
    case OrderOfSCCs::DECREASING:
        sort(sccs.begin(), sccs.end(), compare_sccs_decreasing);
        break;
    case OrderOfSCCs::INCREASING:
        sort(sccs.begin(), sccs.end(), compare_sccs_increasing);
        break;
    }

    /*
      Compute the indices at which the merged SCCs can be found when all
      SCCs have been merged.
    */
    int index = num_vars - 1;
    cout << "SCCs of the causal graph:" << endl;
    vector<vector<int>> non_singleton_cg_sccs;
    vector<int> indices_of_merged_sccs;
    indices_of_merged_sccs.reserve(sccs.size());
    for (const vector<int> &scc : sccs) {
        cout << scc << endl;
        int scc_size = scc.size();
        if (scc_size == 1) {
            indices_of_merged_sccs.push_back(scc.front());
        } else {
            index += scc_size - 1;
            indices_of_merged_sccs.push_back(index);
            non_singleton_cg_sccs.push_back(scc);
        }
    }
    if (sccs.size() == 1) {
        cout << "Only one single SCC" << endl;
    }
    if (static_cast<int>(sccs.size()) == num_vars) {
        cout << "Only singleton SCCs" << endl;
        assert(non_singleton_cg_sccs.empty());
    }

    if (merge_selector) {
        merge_selector->initialize(fts_task);
    }

    return utils::make_unique_ptr<MergeStrategySCCs>(
        fts,
        fts_task,
        merge_tree_factory,
        merge_selector,
        move(non_singleton_cg_sccs),
        move(indices_of_merged_sccs));
}

bool MergeStrategyFactorySCCs::requires_init_distances() const {
    if (merge_tree_factory) {
        return merge_tree_factory->requires_init_distances();
    } else {
        return merge_selector->requires_init_distances();
    }
}

bool MergeStrategyFactorySCCs::requires_goal_distances() const {
    if (merge_tree_factory) {
        return merge_tree_factory->requires_goal_distances();
    } else {
        return merge_selector->requires_goal_distances();
    }
}

void MergeStrategyFactorySCCs::dump_strategy_specific_options() const {
    cout << "Merge order of sccs: ";
    switch (order_of_sccs) {
    case OrderOfSCCs::TOPOLOGICAL:
        cout << "topological";
        break;
    case OrderOfSCCs::REVERSE_TOPOLOGICAL:
        cout << "reverse topological";
        break;
    case OrderOfSCCs::DECREASING:
        cout << "decreasing";
        break;
    case OrderOfSCCs::INCREASING:
        cout << "increasing";
        break;
    }
    cout << endl;

    cout << "Merge strategy for merging within sccs: " << endl;
    if (merge_tree_factory) {
        merge_tree_factory->dump_options();
    }
    if (merge_selector) {
        merge_selector->dump_options();
    }
}

string MergeStrategyFactorySCCs::name() const {
    return "sccs";
}

static shared_ptr<MergeStrategyFactory>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Merge strategy SSCs",
        "This merge strategy implements the algorithm described in the paper "
        + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
            "https://ai.dmi.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
            "Proceedings of the 26th International Conference on Planning and "
            "Scheduling (ICAPS 2016)",
            "2358-2366",
            "AAAI Press 2016") +
        "In a nutshell, it computes the maximal SCCs of the causal graph, "
        "obtaining a partitioning of the task's variables. Every such "
        "partition is then merged individually, using the specified fallback "
        "merge strategy, considering the SCCs in a configurable order. "
        "Afterwards, all resulting composite abstractions are merged to form "
        "the final abstraction, again using the specified fallback merge "
        "strategy and the configurable order of the SCCs.");
    vector<string> order_of_sccs;
    order_of_sccs.push_back("topological");
    order_of_sccs.push_back("reverse_topological");
    order_of_sccs.push_back("decreasing");
    order_of_sccs.push_back("increasing");
    parser.add_enum_option(
        "order_of_sccs",
        order_of_sccs,
        "choose an ordering of the SCCs: topological/reverse_topological or "
        "decreasing/increasing in the size of the SCCs. The former two options "
        "refer to the directed graph where each obtained SCC is a "
        "'supervertex'. For the latter two options, the tie-breaking is to "
        "use the topological order according to that same graph of SCC "
        "supervertices.",
        "topological");
    parser.add_option<shared_ptr<MergeTreeFactory>>(
        "merge_tree",
        "the fallback merge strategy to use if a precomputed strategy should "
        "be used.",
        options::OptionParser::NONE);
    parser.add_option<shared_ptr<MergeSelector>>(
        "merge_selector",
        "the fallback merge strategy to use if a stateless strategy should "
        "be used.",
        options::OptionParser::NONE);

    options::Options options = parser.parse();
    if (parser.help_mode()) {
        return nullptr;
    } else if (parser.dry_run()) {
        bool merge_tree = options.contains("merge_tree");
        bool merge_selector = options.contains("merge_selector");
        if ((merge_tree && merge_selector) || (!merge_tree && !merge_selector)) {
            cerr << "You have to specify exactly one of the options merge_tree "
                "and merge_selector!" << endl;
            utils::exit_with(utils::ExitCode::INPUT_ERROR);
        }
        return nullptr;
    } else {
        return make_shared<MergeStrategyFactorySCCs>(options);
    }
}

static options::PluginShared<MergeStrategyFactory> _plugin("mas_merge_sccs", _parse);
}
