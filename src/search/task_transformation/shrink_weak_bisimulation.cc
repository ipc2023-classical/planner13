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
    int h_and_goal; // -1 for goal states; h value for non-goal states
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h, bool is_goal, int group_,
              const SuccessorSignature &succ_signature_,
              int state_)
        : group(group_), succ_signature(succ_signature_), state(state_) {
        if (is_goal) {
            assert(h == 0);
            h_and_goal = -1;
        } else {
            h_and_goal = h;
        }
    }

    bool operator<(const Signature &other) const {
        if (h_and_goal != other.h_and_goal)
            return h_and_goal < other.h_and_goal;
        if (group != other.group)
            return group < other.group;
        if (succ_signature != other.succ_signature)
            return succ_signature < other.succ_signature;
        return state < other.state;
    }

    void dump() const {
        cout << "Signature(h_and_goal = " << h_and_goal
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
        preserve_optimality (opts.get<bool>("preserve_optimality")) {
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

    StateEquivalenceRelation
    ShrinkWeakBisimulation::compute_equivalence_relation(const FactoredTransitionSystem &fts,
                                                         int index, int /*target*/) const {
        const TransitionSystem &ts = fts.get_ts(index);
        int num_states = ts.get_size();

        vector<vector<int>> non_tau_graph(num_states);
        vector<vector<int>> tau_graph(num_states);
        for (const GroupAndTransitions &gat : ts) {
            const vector<Transition> &transitions = gat.transitions;


            bool is_tau = std::any_of(gat.label_group.begin(), gat.label_group.end(),
                                      [&](int label) {
                                          return fts.is_tau_label(index, LabelID(label)) &&
                                          (!preserve_optimality ||
                                           fts.get_labels().get_label_cost(label) == 0);
                                      });

            if(is_tau) {
                // cout << "Tau label group!" << endl;
                for (const Transition &transition : transitions) {
                    tau_graph[transition.target].push_back(transition.src);
                }
            } else {
                for (const Transition &transition : transitions) {
                    non_tau_graph[transition.target].push_back(transition.src);
                }
            }


            /* remove duplicates in adjacency matrix */
            for (int i = 0; i < num_states; i++) {
                ::sort(tau_graph[i].begin(), tau_graph[i].end());
                auto it = unique(tau_graph[i].begin(), tau_graph[i].end());
                tau_graph[i].erase(it, tau_graph[i].end());


                ::sort(non_tau_graph[i].begin(), non_tau_graph[i].end());
                it = unique(non_tau_graph[i].begin(), non_tau_graph[i].end());
                non_tau_graph[i].erase(it, non_tau_graph[i].end());
            }
        }


        // breadth_first_search to find goal distances
        int current_distance = 0;
        vector<int> goal_distances(num_states, std::numeric_limits<int>::max());
        vector<int> current_queue, next_queue;
        for (int state = 0; state < num_states; ++state) {
            if (ts.is_goal_state(state)) {
                goal_distances[state] = 0;
                next_queue.push_back(state);
            }
        }
        while (!next_queue.empty()) {
            next_queue.swap(current_queue);
            next_queue.clear();
            for(size_t i = 0; i < current_queue.size(); ++i) {
                int state = current_queue[i];
                for (int successor : tau_graph[state]) {
                    if (goal_distances[successor] > current_distance) {
                        goal_distances[successor] = current_distance;
                        current_queue.push_back(successor);
                    }
                }
            }

            for(int state : current_queue) {
                for (int successor : non_tau_graph[state]) {
                    if (goal_distances[successor] > current_distance + 1) {
                        goal_distances[successor] = current_distance + 1;
                        next_queue.push_back(successor);
                    }
                }
            }

            current_distance ++;
        }

        vector<vector<int>> can_reach_via_tau_path(num_states);
        for(int i = 0; i < num_states; ++i) {
            dfs_reachability(tau_graph, i, can_reach_via_tau_path);
        }

        vector<int> state_to_group(num_states);
        vector<Signature> signatures;
        signatures.reserve(num_states + 2);

        int num_groups = initialize_groups(ts, goal_distances, state_to_group);

        bool stable = false;
        while (!stable) {
            stable = true;

            signatures.clear();
            compute_signatures(ts, goal_distances, signatures,
                               state_to_group,
                               can_reach_via_tau_path);

            // Verify size of signatures and presence of sentinels.
            assert(static_cast<int>(signatures.size()) == num_states + 2);
            assert(signatures[0].h_and_goal == -2);
            assert(signatures[num_states + 1].h_and_goal == SENTINEL);

            int sig_start = 1; // Skip over initial sentinel.
            while (true) {
                int h_and_goal = signatures[sig_start].h_and_goal;
                if (h_and_goal == SENTINEL) {
                    // We have hit the end sentinel.
                    assert(sig_start + 1 == static_cast<int>(signatures.size()));
                    break;
                }

                // Compute the number of groups needed after splitting.
                int num_old_groups = 0;
                int num_new_groups = 0;
                int sig_end;
                for (sig_end = sig_start; true; ++sig_end) {
                    if (signatures[sig_end].h_and_goal != h_and_goal) {
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
                        state_to_group[curr_sig.state] = new_group_no;
                    }
                }
                sig_start = sig_end;
            }
        }

        /* Reduce memory pressure before generating the equivalence
           relation since this is one of the code parts relevant to peak
           memory. */
        utils::release_vector_memory(signatures);

        // Generate final result.
        StateEquivalenceRelation equivalence_relation;
        equivalence_relation.resize(num_groups);
        for (int state = 0; state < num_states; ++state) {
            int group = state_to_group[state];
            if (group != -1) {
                assert(group >= 0 && group < num_groups);
                equivalence_relation[group].push_front(state);
            }
        }

        return equivalence_relation;
    }





    int ShrinkWeakBisimulation::initialize_groups(
        const TransitionSystem &ts,
        const vector<int> &goal_distances,
        vector<int> &state_to_group) const {

        typedef unordered_map<int, int> GroupMap;
        GroupMap h_to_group;
        int num_groups = 1; // Group 0 is for goal states.
        for (int state = 0; state < ts.get_size(); ++state) {
            int h = goal_distances[state];
            if (h == INF) {
                h = IRRELEVANT;
            }
            if (ts.is_goal_state(state)) {
                assert(h == 0);
                state_to_group[state] = 0;
            } else {
                pair<GroupMap::iterator, bool> result = h_to_group.insert(
                    make_pair(h, num_groups));
                state_to_group[state] = result.first->second;
                if (result.second) {
                    // We inserted a new element => a new group was started.
                    ++num_groups;
                }
            }
        }
        return num_groups;
    }


    void ShrinkWeakBisimulation::compute_signatures(
        const TransitionSystem &ts,
        const vector<int> &goal_distances,
        vector<Signature> &signatures,
        const vector<int> &state_to_group,
        const vector<vector<int>> &can_reach_via_tau_path) const {
        assert(signatures.empty());

        // Step 1: Compute bare state signatures (without transition information).
        signatures.push_back(Signature(-2, false, -1, SuccessorSignature(), -1));
        for (int state = 0; state < ts.get_size(); ++state) {
            int h = goal_distances[state];
            if (h == INF) {
                h = IRRELEVANT;
            }
            Signature signature(h, ts.is_goal_state(state),
                                state_to_group[state], SuccessorSignature(),
                                state);
            signatures.push_back(signature);
        }
        signatures.push_back(Signature(SENTINEL, false, -1, SuccessorSignature(), -1));

        // Step 2: Add transition information.
        int label_group_counter = 0;
        for (const GroupAndTransitions &gat : ts) {
            const vector<Transition> &transitions = gat.transitions;
            for (const Transition &transition : transitions) {
                assert(signatures[transition.src + 1].state == transition.src);

                int target_group = state_to_group[transition.target];
                assert(target_group != -1 && target_group != SENTINEL);

                for (int src_state : can_reach_via_tau_path[transition.src]) {
                    assert(signatures[src_state + 1].state == src_state);

                    signatures[src_state + 1].succ_signature.push_back(
                        make_pair(label_group_counter, target_group));
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
                    equivalences[old_index] = compute_equivalence_relation(fts, index, 0);

                    size_t old_size = fts.get_ts(index).get_size();


                    // cout << "Shrinking: " << old_size << " to " << equivalences[old_index].size() << endl;

                    if (equivalences[old_index].size() < old_size) {

                        unique_ptr<TauGraph> tau_graph (new TauGraph(fts, index, preserve_optimality));

                        int succ_index = -1;
                        if (equivalences[old_index].size() > 1) {
                            equivalences_to_apply.push_back(old_index);
                            succ_index = new_index++;
                        } else{
                            assert (equivalences[old_index].size() == 1);
                            exclude_transition_systems.insert(index);
                        }

                        vector<int> abstraction_mapping = compute_abstraction_mapping(old_size, equivalences[old_index]);

                        unique_ptr<TransitionSystem> copy_tr(new TransitionSystem(fts.get_ts(index)));

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
                cout << "Applying changes to " << index << endl;
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

        Options opts = parser.parse();

        if (!parser.dry_run())
            return  make_shared<ShrinkWeakBisimulation>(opts);
        else
            return nullptr;
    }

    static PluginShared<ShrinkStrategy> _plugin("weak_bisimulation", _parse);

    shared_ptr<ShrinkStrategy> ShrinkWeakBisimulation::create_default() {
        Options opts;
        opts.set<bool> ("goal_shrinking", true);
        opts.set<bool> ("preserve_optimality", false);

        return make_shared<ShrinkWeakBisimulation>(opts);
    }

}
