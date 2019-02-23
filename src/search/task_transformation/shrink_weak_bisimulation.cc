	#include "shrink_weak_bisimulation.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "../task_representation/label_equivalence_relation.h"
#include "../task_representation/transition_system.h"
#include "../task_representation/labels.h"
#include "plan_reconstruction_tau_path.h"

#include "tau_graph.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../algorithms/sccs.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <algorithm>

using namespace std;

namespace task_transformation {

    /* A successor signature characterizes the behaviour of an abstract
   state in so far as bisimulation cares about it. States with
   identical successor signature are not distinguished by
   bisimulation.

   Each entry in the vector is a pair of (label group ID, equivalence class of
   successor). The bisimulation algorithm requires that the vector is
   sorted and uniquified. */
using SuccessorSignature = vector<pair<int, int>>;

/*
  As we use SENTINEL numeric_limits<int>::max() as a sentinel signature and
  irrelevant states have a distance of INF = numeric_limits<int>::max(), we
  use INF - 1 as the distance value for all irrelevant states. This guarantees
  that also irrelevant states are always ordered before the sentinel.
*/
const int SENTINEL = numeric_limits<int>::max();
const int IRRELEVANT = SENTINEL - 1;

/*
  The following class encodes all we need to know about a state for
  bisimulation: its h value, which equivalence class ("group") it currently
  belongs to, its successor signature (see above), and what the original
  state is.
*/

struct Signature {
    int h; // h value (tau transitions cost 0, other transitions cost 1)
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h_, int group_,
              const SuccessorSignature &succ_signature_,
              int state_)
        : h(h_), group(group_), succ_signature(succ_signature_), state(state_) {
    }

    bool operator<(const Signature &other) const {
        if (h != other.h)
            return h < other.h;
        if (group != other.group)
            return group < other.group;
        if (succ_signature != other.succ_signature)
            return succ_signature < other.succ_signature;
        return state < other.state;
    }

    void dump() const {
        cout << "Signature(h = " << h
             << ", group = " << group
             << ", state = " << state
             << ", succ_sig = [";
        for (size_t i = 0; i < succ_signature.size(); ++i) {
            if (i)
                cout << ", ";
            cout << "(" << succ_signature[i].first
                 << "," << succ_signature[i].second
                 << ")";
        }
        cout << "])" << endl;
    }
};

    ShrinkWeakBisimulation::ShrinkWeakBisimulation(const Options &opts) :
        ShrinkStrategy(),
        preserve_optimality (opts.get<bool>("preserve_optimality")),
        ignore_irrelevant_tau_groups (opts.get<bool>("ignore_irrelevant_tau_groups")){
    }

    void ShrinkWeakBisimulation::dump_strategy_specific_options() const {
        cout << "Preserve optimallity: " <<
            (preserve_optimality ? "yes" : "no") << endl;
    }


    static void dfs_reachability(const vector<vector<int>> & tau_graph,
                                 int i,
                                 vector<vector<int>> & can_reach_via_tau_path){
        if(!can_reach_via_tau_path[i].empty()) {
            return;
        }

        can_reach_via_tau_path[i].push_back(i);

        for (int predecessor : tau_graph[i]) {
            dfs_reachability(tau_graph, predecessor, can_reach_via_tau_path);
            for (int s : can_reach_via_tau_path[predecessor]) {
                can_reach_via_tau_path[i].push_back(s);
            }
        }

        ::sort(can_reach_via_tau_path[i].begin(), can_reach_via_tau_path[i].end());
        vector<int>::iterator it = unique(can_reach_via_tau_path[i].begin(), can_reach_via_tau_path[i].end());
        can_reach_via_tau_path[i].erase(it, can_reach_via_tau_path[i].end());
    }

            /* remove duplicates in adjacency matrix */

    static void sort_unique (vector<vector<int>> & vs) {
        for (auto & v : vs) {
            ::sort(v.begin(), v.end());
            v.erase(unique(v.begin(), v.end()), v.end());
        }
    }

    StateEquivalenceRelation
    ShrinkWeakBisimulation::compute_equivalence_relation(const FactoredTransitionSystem &fts,
                                                         int index, int /*target*/) const {
        const TransitionSystem &ts = fts.get_ts(index);
        int num_states = ts.get_size();

        vector<bool> ignore_label_group(ts.num_label_groups(), false);

        // Step 1: Compute tau graph
        vector<vector<int>> tau_graph(num_states);
        int label_group_index = 0;
        for (const GroupAndTransitions &gat : ts) {
            const vector<Transition> &transitions = gat.transitions;

            bool is_tau = std::any_of(gat.label_group.begin(), gat.label_group.end(),
                                      [&](int label) {
                                          return fts.is_tau_label(index, LabelID(label)) &&
                                          (!preserve_optimality ||
                                           fts.get_labels().get_label_cost(label) == 0);
                                      });

            if(is_tau) {
                ignore_label_group [label_group_index] = ignore_irrelevant_tau_groups &&
                    std::all_of(gat.label_group.begin(), gat.label_group.end(),
                                [&](int label) {
                                    return !fts.is_tau_label(index, LabelID(label));
                                });

                // cout << "Tau label group!" << endl;
                for (const Transition &transition : transitions) {
                    tau_graph[transition.target].push_back(transition.src);
                }
            }

            label_group_index ++;

        }
        sort_unique(tau_graph);


        //Step 2: Compute SCCs in tau graph
        StateEquivalenceRelation final_sccs;
        sccs::SCC<int>::compute_scc_equivalence (tau_graph, final_sccs);
        int num_sccs = final_sccs.size();
        vector<int> mapping_to_scc = compute_abstraction_mapping (num_states, final_sccs);
        assert (count(mapping_to_scc.begin(), mapping_to_scc.end(), -1) == 0);

        //Step 3: Compute goal distances (on the SCC graph)
        vector<vector<int>> tau_scc_graph(num_sccs);
        std::vector<bool> is_scc_goal (num_sccs, false);
        for (int s = 0; s < num_states; ++s) {
            for (int t : tau_graph[s]) {
                tau_scc_graph[mapping_to_scc[s]].push_back(mapping_to_scc[t]);
            }
            if (ts.is_goal_state(s)) {
                is_scc_goal[mapping_to_scc[s]] = true;
            }
        }
        sort_unique(tau_scc_graph);

        vector<vector<int>> non_tau_scc_graph(num_sccs);
        for (const GroupAndTransitions &gat : ts) {
            const vector<Transition> &transitions = gat.transitions;

            bool is_tau = std::any_of(gat.label_group.begin(), gat.label_group.end(),
                                      [&](int label) {
                                          return fts.is_tau_label(index, LabelID(label)) &&
                                          (!preserve_optimality ||
                                           fts.get_labels().get_label_cost(label) == 0);
                                      });

            if(!is_tau) {
                // cout << "Tau label group!" << endl;
                for (const Transition &transition : transitions) {
                    non_tau_scc_graph[mapping_to_scc[transition.target]].push_back(mapping_to_scc[transition.src]);
                }
            }
        }
        sort_unique(non_tau_scc_graph);

        // breadth_first_search to find goal distances
        int current_distance = 0;
        vector<int> goal_distances(num_sccs, std::numeric_limits<int>::max());
        vector<int> current_queue, next_queue;
        for (int scc = 0; scc < num_sccs; ++scc) {
            if (is_scc_goal[scc]) {
                goal_distances[scc] = 0;
                next_queue.push_back(scc);
            }
        }
        while (!next_queue.empty()) {
            next_queue.swap(current_queue);
            next_queue.clear();
            for(size_t i = 0; i < current_queue.size(); ++i) {
                int state = current_queue[i];
                for (int successor : tau_scc_graph[state]) {
                    if (goal_distances[successor] > current_distance) {
                        goal_distances[successor] = current_distance;
                        current_queue.push_back(successor);
                    }
                }
            }

            for(int state : current_queue) {
                for (int successor : non_tau_scc_graph[state]) {
                    if (goal_distances[successor] > current_distance + 1) {
                        goal_distances[successor] = current_distance + 1;
                        next_queue.push_back(successor);
                    }
                }
            }

            current_distance ++;
        }

        //Step 4: Compute can_reach_via_tau_path
        vector<vector<int>> can_reach_via_tau_path(num_sccs);
        for(int i = 0; i < num_sccs; ++i) {
            dfs_reachability(tau_scc_graph, i, can_reach_via_tau_path);
        }

        //Step 5: Initialize Weak Bisimulations with the goal distances
        vector<int> scc_to_group(num_sccs);
        vector<Signature> signatures;
        signatures.reserve(num_sccs + 2);

        int num_groups = initialize_groups(goal_distances, scc_to_group);

        //Step 6: Compute weak bisimulation
        bool stable = false;
        while (!stable) {
            stable = true;

            signatures.clear();
            compute_signatures(ts, mapping_to_scc, goal_distances, ignore_label_group, signatures, scc_to_group, can_reach_via_tau_path);

            // Verify size of signatures and presence of sentinels.
            assert(static_cast<int>(signatures.size()) == num_sccs + 2);
            assert(signatures[0].h == -2);
            assert(signatures[num_sccs + 1].h == SENTINEL);

            int sig_start = 1; // Skip over initial sentinel.
            while (true) {
                int h = signatures[sig_start].h;
                if (h == SENTINEL) {
                    // We have hit the end sentinel.
                    assert(sig_start + 1 == static_cast<int>(signatures.size()));
                    break;
                }

                // Compute the number of groups needed after splitting.
                int num_old_groups = 0;
                int num_new_groups = 0;
                int sig_end;
                for (sig_end = sig_start; true; ++sig_end) {
                    if (signatures[sig_end].h != h) {
                        break;
                    }

                    const Signature &prev_sig = signatures[sig_end - 1];
                    const Signature &curr_sig = signatures[sig_end];

                    if (sig_end == sig_start) {
                        assert(prev_sig.group != curr_sig.group);
                    }

                    if (prev_sig.group != curr_sig.group) {
                        ++num_old_groups;
                        ++num_new_groups;
                    } else if (prev_sig.succ_signature != curr_sig.succ_signature) {
                        ++num_new_groups;
                    }
                }
                assert(sig_end > sig_start);

                if (num_new_groups != num_old_groups) {
                    // Split into new groups.
                    stable = false;

                    int new_group_no = -1;
                    for (int i = sig_start; i < sig_end; ++i) {
                        const Signature &prev_sig = signatures[i - 1];
                        const Signature &curr_sig = signatures[i];

                        if (prev_sig.group != curr_sig.group) {
                            // Start first group of a block; keep old group no.
                            new_group_no = curr_sig.group;
                        } else if (prev_sig.succ_signature
                                   != curr_sig.succ_signature) {
                            new_group_no = num_groups++;
                        }

                        assert(new_group_no != -1);
                        scc_to_group[curr_sig.state] = new_group_no;
                    }
                }
                sig_start = sig_end;
            }
        }

        /* Reduce memory pressure before generating the equivalence relation since this is
           one of the code parts relevant to peak memory. */
        utils::release_vector_memory(signatures);


        // Step 7: Generate final result.
        StateEquivalenceRelation equivalence_relation;
        equivalence_relation.resize(num_groups);
        for (int state = 0; state < num_states; ++state) {
            int group = scc_to_group[mapping_to_scc[state]];
            if (group != -1) {
                assert(group >= 0 && group < num_groups);
                equivalence_relation[group].push_front(state);
            }
        }

        return equivalence_relation;
    }





    int ShrinkWeakBisimulation::initialize_groups(
        const vector<int> &goal_distances,
        vector<int> &state_to_group) const {

        typedef unordered_map<int, int> GroupMap;
        GroupMap h_to_group;
        int num_groups = 0;
        vector<int> irrelevant_states;
        for (size_t state = 0; state < goal_distances.size(); ++state) {
            int h = goal_distances[state];
            if (h == INF) {
                irrelevant_states.push_back(state);
            } else {
                state_to_group[state] = h;
                num_groups = max(num_groups, h);
            }
        }
        if (irrelevant_states.size()) {
            num_groups ++;
            for (int s : irrelevant_states) {
                state_to_group[s] = num_groups;
            }
        }
        return num_groups + 1;
    }

    void ShrinkWeakBisimulation::compute_signatures(
        const TransitionSystem &ts,
        const vector<int> & mapping_to_scc,
        const vector<int> &goal_distances,
        const vector<bool> &ignore_label_group,
        vector<Signature> &signatures,
        const vector<int> &state_to_group,
        const vector<vector<int>> &can_reach_via_tau_path) const {
        assert(signatures.empty());

        // Step 1: Compute bare state signatures (without transition information).
        signatures.push_back(Signature(-2, -1, SuccessorSignature(), -1));
        for (size_t state = 0; state < goal_distances.size(); ++state) {
            int h = goal_distances[state];
            if (h == INF) {
                h = IRRELEVANT;
            }
            signatures.push_back(Signature (h, state_to_group[state], SuccessorSignature(), state));
        }
        signatures.push_back(Signature(SENTINEL, -1, SuccessorSignature(), -1));

        // Step 2: Add transition information.
        int label_group_counter = 0;
        for (const GroupAndTransitions &gat : ts) {
            if (!ignore_label_group[label_group_counter]) {
                const vector<Transition> &transitions = gat.transitions;
                for (const Transition &transition : transitions) {
                    int transition_src = mapping_to_scc[transition.src];
                    int transition_target = mapping_to_scc[transition.target];

                    assert(signatures[transition_src + 1].state == transition_src);

                    int target_group = state_to_group[transition_target];
                    assert(target_group != -1 && target_group != SENTINEL);

                    for (int src_state : can_reach_via_tau_path[transition_src]) {
                        assert (src_state >= 0);
                        assert (src_state < (int)(goal_distances.size()));
                        assert(signatures[src_state + 1].state == src_state);

                        signatures[src_state + 1].succ_signature.push_back(
                            make_pair(label_group_counter, target_group));
                    }
                }
            }
            ++label_group_counter;
        }

        /* Step 3: Canonicalize the representation. The resulting signatures must satisfy
           the following properties:
           1. Signature::operator< defines a total order with the correct sentinels at the
           start and end. The signatures vector is sorted according to that order.
           2. Goal states come before non-goal states, and low-h states come before high-h states.
           3. States that currently fall into the same group form contiguous subsequences.
           4. Two signatures compare equal according to Signature::operator< iff we don't
           want to distinguish their states in the current bisimulation round.
        */
        for (size_t i = 0; i < signatures.size(); ++i) {
            SuccessorSignature &succ_sig = signatures[i].succ_signature;
            ::sort(succ_sig.begin(), succ_sig.end());
            succ_sig.erase(::unique(succ_sig.begin(), succ_sig.end()),
                           succ_sig.end());
        }

        ::sort(signatures.begin(), signatures.end());
    }


    bool ShrinkWeakBisimulation::apply_shrinking_transformation(FactoredTransitionSystem & fts,
                                                                Verbosity verbosity,
                                                                int &  check_only_index) const  {

        assert(!fts.remove_irrelevant_labels());

        if (verbosity == Verbosity::VERBOSE) {
            cout << "Apply shrinking transformation over ";
            if(check_only_index >= 0) {
                cout << check_only_index << "\n";
            } else {
                cout << "all\n";
            }
        }
        assert (check_only_index < 0 || fts.is_active(check_only_index));

      int old_index = 0;
      int new_index = 0;
      vector<int> initial_state_values;

      std::set<int> exclude_transition_systems;
      // All equivalences must be applied after computing the equivalences for other FTSs
      vector<StateEquivalenceRelation> equivalences (fts.get_size());
      vector<int> equivalences_to_apply;
      vector<unique_ptr<TauShrinking>> tau_shrinking_reconstruction;

      for (int index = 0; index < fts.get_size(); ++index) {
            if (fts.is_active(index)) {
                initial_state_values.push_back(fts.get_ts(index).get_init_state());

                if (check_only_index == -1 || index == check_only_index) {
                    equivalences[new_index] = compute_equivalence_relation(fts, index, 0);
                    assert(equivalences[new_index].size() > 0);

                    size_t old_size = fts.get_ts(index).get_size();

                    if (verbosity == Verbosity::VERBOSE) {
                        cout << "Weak bisimulation shrinking from " << old_size << " to " << equivalences[new_index].size() << endl;
                    }

                    if (equivalences[new_index].size() < old_size) {
                        vector<int> abstraction_mapping = compute_abstraction_mapping(old_size, equivalences[new_index]);
                        assert (count(abstraction_mapping.begin(), abstraction_mapping.end(), -1) == 0);


                        int succ_index = -1;
                        if (equivalences[new_index].size() > 1) {
                            equivalences_to_apply.push_back(new_index);
                            succ_index = new_index++;
                        } else{
                            assert (equivalences[new_index].size() == 1);
                            if (verbosity == Verbosity::VERBOSE) {
                                cout << "Transition system " << index << " removed by weak bisimulation" << endl;
                            }
                            exclude_transition_systems.insert(index);
                        }

                        unique_ptr<TransitionSystem> copy_tr(new TransitionSystem(fts.get_ts(index)));

                        unique_ptr<TauGraph> tau_graph (new TauGraph(fts, index, preserve_optimality));
                        tau_shrinking_reconstruction.push_back(utils::make_unique_ptr<TauShrinking> (old_index, succ_index, move(tau_graph),
                                                                                                     move(abstraction_mapping),
                                                                                                     unique_ptr<TransitionSystem>(new TransitionSystem(fts.get_ts(index)))));
                    } else {
                        new_index ++;
                    }
                } else {
                    new_index++;
                }
                old_index ++;
            }
      }

      bool changes = !(exclude_transition_systems.empty());
      if (!equivalences_to_apply.empty() || !exclude_transition_systems.empty() ){
          cout << "WeakBisimulation applicable in " <<
              (equivalences_to_apply.size() + exclude_transition_systems.size())
               << " out of " << old_index << " systems" << endl;

          // 1) Extract the plan reconstruction M&S and insert it in the list of plan
          // reconstruction steps
          FTSMapping fts_m = fts.cleanup(exclude_transition_systems);
          for (auto & sh : tau_shrinking_reconstruction) {
              sh->apply_label_mapping(fts_m.label_mapping);
          }
          assert(fts.get_size() == new_index);


          if (check_only_index >= 0) {
              check_only_index = fts_m.transition_system_all_mapping[check_only_index];
          }

          // 2) Add tau plan reconstruction step
          fts.add_plan_reconstruction(
              make_shared<PlanReconstructionTauPath>(fts_m, PlanState(move(initial_state_values)),
                                                     move(tau_shrinking_reconstruction)));


          // 3) Apply abstractions:
          for (int index : equivalences_to_apply) {
              cout << "Applying changes to " << index << " out of " << fts.get_size() << endl;
              assert (check_only_index < 0 || check_only_index == index);
              assert (equivalences[index].size() > 0);
              changes |= fts.apply_abstraction(index, equivalences[index], verbosity, true);
          }

          // 4) Re-initialize fts with new merge_and_shrink and label mappings
          fts.reinitialize_predecessor_task();


          cout << "There are " << fts.get_size() << " systems after applying weak bisimulation label shrinking"  <<endl;

        }
        return changes;
    }

    bool ShrinkWeakBisimulation::apply_shrinking_transformation(FactoredTransitionSystem &fts, Verbosity verbosity) const  {

        int index = -1;
        return apply_shrinking_transformation(fts, verbosity, index);
    }


    std::string ShrinkWeakBisimulation::name() const {
        return "weak_bisimulation";
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

        parser.add_option<bool>("preserve_optimality",
                                "Only consider tau transitions with 0-cost actions so that the reduction is optimallity preserving",
                                "false");

        parser.add_option<bool>("ignore_irrelevant_tau_groups",
                                "During weak bisimulation, label groups that are tau and externally irrelevant are ignored",
                                "false");

        Options opts = parser.parse();

        if (!parser.dry_run())
            return  make_shared<ShrinkWeakBisimulation>(opts);
        else
            return nullptr;
    }

    static PluginShared<ShrinkStrategy> _plugin("shrink_weak_bisimulation", _parse);

    shared_ptr<ShrinkStrategy> ShrinkWeakBisimulation::create_default() {
        Options opts;
        opts.set<bool> ("preserve_optimality", false);
        opts.set<bool> ("ignore_irrelevant_tau_groups", false);

        return make_shared<ShrinkWeakBisimulation>(opts);
    }

}
