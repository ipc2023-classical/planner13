#include "shrink_own_labels.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "../task_representation/label_equivalence_relation.h"
#include "../task_representation/transition_system.h"
#include "../task_representation/labels.h"


#include "../option_parser.h"
#include "../plugin.h"

#include "../algorithms/sccs.h"

#include <cassert>
#include <iostream>
#include <algorithm>

using namespace std;

namespace task_transformation {

ShrinkOwnLabels::ShrinkOwnLabels(const Options &opts) : 
    ShrinkStrategy(), 
    perform_sg_shrinking (opts.get<bool>("goal_shrinking")) ,
    preserve_optimality (opts.get<bool>("preserve_optimality")) {
}


ShrinkOwnLabels::~ShrinkOwnLabels() {
}

// string ShrinkOwnLabels::name() const {
//     return "own labels (to identify unsol. tasks)";
// }

void ShrinkOwnLabels::dump_strategy_specific_options() const {
    cout << "Aggregate with goal states: " << 
	(perform_sg_shrinking? "yes" : "no") << endl;
}

    

   
StateEquivalenceRelation ShrinkOwnLabels::compute_equivalence_relation(
    const FactoredTransitionSystem &fts,
    int index,
    int /*target*/) const {
    /*
     * apply two rules:
     * (1) aggregate all states s1, ..., sn if they lie on an own-label cycle
     *     idea: use Tarjan's algorithm to identify strongly connected components (SCCs)
     * (2) aggregate state s with goal state g if:
     *     (a) perform_sg_shrinking is activated
     *     (b) all goal variables are in abstraction and
     *     (c) there is an own-label path from s to g
     *     idea: set goal-status for all states already during SCC detection
     *     PETER: why only aggregate s with g and not all states that
     *     are marked as goal states?
     *     PETER: when we are at it, why not remove all outgoing
     *            transitions of goal states (we cannot leave them
     *            anyway any more)?
     *     PETER: actually, if we use the first optimization, we must
     *            use the second one as well, as otherwise unreachable
     *            states might suddenly become reachable resulting in
     *            an immense overhead in abstract states
     *     PETER: we might actually do this as a general optimization
     *     in Abstraction::normalize: whenever all goal variables are
     *     merged in, we can safely remove any transitions starting at
     *     goal states
     *     PETER: removing goal transitions is not safe.
     * In case of own-label shrinking we inherit the information of
     * being a goal state from the own-label reachable
     * successors. If this way the initial state is marked as a goal,
     * but some states are not marked as goal because their outgoing
     * actions are not yet own-labeled, then removing the outgoing
     * actions of the goal states will mean that those states will
     * become unreachable and thus they will be pruned
     * CONCLUSION PETER: aggregating all goal-states is added as an
     * option in bisimulation, cause the abstraction size may increase
     * due to less reachability pruning. Pruning goal transitions is
     * disabled because is not safe if we later perform search
     */

    const TransitionSystem & ts = fts.get_ts(index);

    int num_states = ts.get_size();
    std::vector<bool> is_goal (ts.get_is_goal());
    cout << "Applying OwnLabel shrinking to ts: " << index << endl;
                                                              
    /* this is a rather memory-inefficient way of implementing Tarjan's algorithm, but
       it's the best I got for now */
    vector<vector<int> > adjacency_matrix(num_states);
    for (const auto & gt : ts) {
	bool is_own = std::any_of(gt.label_group.begin(), gt.label_group.end(),
				  [&](int label) {
				      return fts.is_tau_label(index, LabelID(label)) &&
                                      (!preserve_optimality ||
                                       fts.get_labels().get_label_cost(label) == 0);
			       });
	if(is_own) {
	    for (const auto & trans : gt.transitions) {
		adjacency_matrix[trans.src].push_back(trans.target);
		// cout << trans.src << " -> " << trans.target << endl;
	    }
	}
    }
    
    /* PETER: Can we do better than this, i.e., prevent the sorting? */
    /* remove duplicates in adjacency matrix */
    for (int i = 0; i < num_states; i++) {
        ::sort(adjacency_matrix[i].begin(), adjacency_matrix[i].end());
        vector<int>::iterator it = unique(adjacency_matrix[i].begin(), adjacency_matrix[i].end());
        adjacency_matrix[i].erase(it, adjacency_matrix[i].end());
    }

    /* perform Tarjan's algorithm for finding SCCs */
    StateEquivalenceRelation final_sccs;
    sccs::SCC::compute_scc_equivalence (adjacency_matrix, final_sccs, &is_goal);

   // cout << "===========================================" << endl;
   //    for (int i = 0; i < num_states; i++) {
   //    cout << "edges from " << i << " to";
   //    for (size_t j = 0; j < adjacency_matrix[i].size(); j++)
   //    cout << " " << adjacency_matrix[i][j];
   //    cout << endl;
   //    }
   //    cout << "found SCCs:" << endl;
   //    for (auto & scc : final_sccs){
   //        cout << std::count_if(begin(scc), end(scc), [](int){ return true; }) << ": " ;
   //        for (int val : scc)  cout << val << " ";
   //        cout << endl;
   //    }
   //    cout << "===========================================" << endl;
    /* free some memory */
    vector<vector<int> > ().swap(adjacency_matrix);

    int new_size = final_sccs.size();
    if (perform_sg_shrinking && fts.is_only_goal_relevant(index)) {
        /* now bring those groups together that follow the second rule */
        cout << "also using second rule of own-label shrinking" << endl;
        int goal_scc = -1;
        for (size_t i = 0; i < final_sccs.size(); i++) {
	    if (is_goal[final_sccs[i].front()]) {
		if (goal_scc == -1) {
		    goal_scc = i;
		} else {
		    final_sccs[goal_scc].splice_after(final_sccs[goal_scc].begin(), final_sccs[i]);
		    new_size--;
		}
	    }
        }
    }


    
    // only need to apply abstraction if this actually changes anything
    StateEquivalenceRelation equivalence_relation;
    equivalence_relation.resize(new_size);
    int counter = 0;
    for (size_t group = 0; group < final_sccs.size(); ++group) {
	if (final_sccs[group].empty())
	    continue;
	equivalence_relation[counter].swap(final_sccs[group]);
	counter++;
    }
#ifndef NDEBUG
    if (new_size < num_states) {
	cout << "Own-label shrinking reduces the number of states" << endl;
    }
#endif


    return equivalence_relation;
}

std::string ShrinkOwnLabels::name() const {
    return "own_labels";
}

static shared_ptr<ShrinkStrategy> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Own-label based shrink strategy",
        "This shrink strategy implements the algorithm described in the paper:\n\n"
        " * Joerg Hoffmann, Peter Kissmann and Alvaro Torralba.<<BR>>\n"
        " [\"Distance\"? Who Cares? Tailoring Merge-and-Shrink Heuristics to Detect Unsolvability "
        "http://fai.cs.uni-saarland.de/hoffmann/papers/ecai14a.pdf].<<BR>>\n "
        "In //Proceedings of the 21st European Conference on Artificial Intelligence (ECAI 14) "
        "//, pp. 441-446. 2014.");

    parser.add_option<bool>("goal_shrinking",
                            "performs goal shrinking. Aggregate state s with goal state g if:"
			    "   (a) this parameter is activated"
			    "   (b) all goal variables are in abstraction and"
			    "   (c) there is an own-label path from s to g",
                            "true");
    
    parser.add_option<bool>("preserve_optimality",
                            "Only consider tau transitions with 0-cost actions so that the reduction is optimallity preserving",
                            "false");

    Options opts = parser.parse();

    if (!parser.dry_run())
	return  make_shared<ShrinkOwnLabels>(opts);
    else
	return nullptr;
}

static PluginShared<ShrinkStrategy> _plugin("own_labels", _parse);

shared_ptr<ShrinkStrategy> ShrinkOwnLabels::create_default() {
    Options opts; 
    opts.set<bool> ("goal_shrinking", true);

    return make_shared<ShrinkOwnLabels>(opts);

}

}
