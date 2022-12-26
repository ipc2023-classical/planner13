#include "numeric_dominance_fts_pruning.h"

#include "dominance_function_builder.h"
#include "../task_transformation/factored_transition_system.h"
#include "../options/options.h"
#include "../options/option_parser.h"
#include "../plugin.h"
#include "../globals.h"

#include <set>
#include <memory>

using namespace std;

namespace dominance {

    template<typename TCost>
    std::vector<int> NumericDominanceFTSPruning<TCost>::prune_transitions(FactoredTransitionSystem &fts) const {
        return prune_dominated_transitions(fts, dominance_function_builder->compute_dominance_function<TCost>(fts));
    }

    template<typename TCost>
    std::vector<int> NumericDominanceFTSPruning<TCost>::prune_dominated_transitions(FactoredTransitionSystem &fts,
                                                                                std::shared_ptr<DominanceFunction<TCost>> ndrel) const {
        assert(fts.get_num_active_entries() == (int)(ndrel->size()));

        const Labels &labels = fts.get_labels();
        const LabelDominanceFunction<TCost> label_relation = ndrel->get_label_relation();

        // Ordered by transition system -> labelId -> list of transitions
        vector<unordered_map<int, set<Transition>>> label_transitions_to_be_removed(fts.get_size());

        // Recompute distances for these transition systems
        unordered_set<int> affected_ts_ids;

        int num_pruned_transitions = 0;
        // A simplifying local function to add transitions to be removed
        auto add_transition_to_be_removed = [&](int ts_id, int l_id, Transition tr) {
            if (label_transitions_to_be_removed[ts_id].find(l_id) == label_transitions_to_be_removed[ts_id].end()) {
                label_transitions_to_be_removed[ts_id][l_id] = set<Transition>();
            }
            num_pruned_transitions++;
            affected_ts_ids.insert(ts_id);

            label_transitions_to_be_removed[ts_id][l_id].insert(tr);
        };


        // A) remove transitions that are dominated by noop in a transition system
        // Labels to completely remove (because they are dominated by noop in all)
        vector<LabelID> labels_to_remove;
        for (LabelID l_id(0); l_id < label_relation.get_num_labels(); ++l_id) {
            if (l_id == 348) {
                std::cout << endl;
            }
            int ts_id = label_relation.get_may_dominated_by_noop_in(l_id);

            if (ts_id >= 0) {
                const TransitionSystem &ts_label_is_dominated_in = fts.get_ts(ts_id);
                if (label_relation.q_dominated_by_noop(l_id, ts_id) >= 0) {
                    for (const Transition &tr: ts_label_is_dominated_in.get_transitions_with_label(l_id)) {
                        if ((*ndrel)[ts_id].simulates(tr.src, tr.target)
                            && ndrel->propagate_transition_pruning(ts_id, ts_label_is_dominated_in, tr.src, l_id,
                                                                   tr.target)) {

                            add_transition_to_be_removed(ts_id, l_id, tr);
                        }
                    }
                }
            } else if (ts_id == DOMINATES_IN_ALL) {
                TCost dom = label_relation.q_dominated_by_noop(l_id);
                if (dom >= 0) {
                    // If it is actually dominated by noop in all, we can remove the label completely
                    labels_to_remove.push_back(l_id);
                }
            }
        }
        vector<int> rd = fts.remove_labels(labels_to_remove);
        affected_ts_ids.insert(rd.begin(), rd.end());

        // B) remove transitions that are dominated by some other transition
        for (int ts_id = 0; ts_id < fts.get_size(); ts_id++) {
            const TransitionSystem &ts = fts.get_ts(ts_id);

            for (LabelGroupID lg1_id(0); lg1_id < ts.num_label_groups(); ++lg1_id) {
                for (LabelGroupID lg2_id(0); lg2_id < ts.num_label_groups(); ++lg2_id) {
                    for (const Transition &tr1: ts.get_transitions_for_group_id(lg1_id)) {
                        const vector<Transition> &trs2 = ts.get_transitions_for_group_id(lg2_id);
                        auto tr2 = std::lower_bound(trs2.begin(), trs2.end(), Transition(tr1.src, 0));
                        auto tr2_end = std::upper_bound(tr2, trs2.end(),
                                                        Transition(tr1.src, std::numeric_limits<int>::max()));

                        for (; tr2 != tr2_end; tr2++) {
                            assert(tr1.src == tr2->src);
                            for (int l1_id: ts.get_label_group(lg1_id)) {
                                for (int l2_id: ts.get_label_group(lg2_id)) {
                                    if (l1_id == l2_id) continue;

                                    // Handle special -infinity cases to avoid underflow
                                    TCost l_qdom = label_relation.q_dominates(LabelID(l1_id), LabelID(l2_id), ts_id);
                                    if (l_qdom == std::numeric_limits<int>::lowest()) continue;

                                    TCost s_qsim = (*ndrel)[ts_id].q_simulates(tr1.target, tr2->target);
                                    if (s_qsim == std::numeric_limits<int>::lowest()) continue;

                                    // Check if src -- l1_id --> tr1.target dominates src -- l2_id --> tr2.target
                                    if (l_qdom + s_qsim - labels.get_label_cost(l1_id) + labels.get_label_cost(l2_id) >=
                                        0) {

                                        if (ndrel->propagate_transition_pruning(ts_id, ts, tr2->src, LabelID(l1_id),
                                                                                tr2->target)) {
                                            std::cout << tr1.src << "--" << l1_id << "->" << tr1.target << " dominates "
                                                      << tr2->src << "--" << l2_id << "->" << tr2->target << std::endl;

                                            add_transition_to_be_removed(ts_id, l2_id, *tr2);
                                            affected_ts_ids.insert(ts_id);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        for (int ts_id = 0; ts_id < fts.get_size(); ts_id++) {
//            assert(is_sorted(ts.get_transitions_for_group_id(lg1_id).begin(), ts.get_transitions_for_group_id(lg1_id).end()));
//
            fts.get_ts_mutable(ts_id).remove_transitions_for_labels(label_transitions_to_be_removed[ts_id]);
        }
        fts.remove_irrelevant_labels();

        std::cout << "Pruned " << num_pruned_transitions << " transitions and " << labels_to_remove.size() << " labels"
                  << endl;

        vector<int> affected_ts_ids_vec;
        affected_ts_ids_vec.reserve(affected_ts_ids.size());
        affected_ts_ids_vec.insert(affected_ts_ids_vec.end(), affected_ts_ids.begin(), affected_ts_ids.end());

        return affected_ts_ids_vec;
    }

    template<typename TCost>
    NumericDominanceFTSPruning<TCost>::NumericDominanceFTSPruning(const options::Options &opts) :
            dominance_function_builder(opts.get<shared_ptr<DominanceFunctionBuilder>>("analysis")),
            prune_transitions_before_main_loop(opts.get<bool>("prune_before")),
            prune_transitions_after_main_loop(opts.get<bool>("prune_after")),
            dump(opts.get<bool>("dump")) {

    }

    template class NumericDominanceFTSPruning<int>;
    template class NumericDominanceFTSPruning<IntEpsilon>;
}

using namespace dominance;
static shared_ptr<dominance::FTSTransitionPruning> _parse(options::OptionParser &parser) {
    parser.document_synopsis("Transformation that eliminates transitions that are dominated by others", "");

    parser.add_option<bool>("dump",
                            "Dumps the relation that has been found",
                            "false");

    parser.add_option<bool>("prune_before",
                            "Do pruning before main loop of MAS",
                            "true");

    parser.add_option<bool>("prune_after",
                            "Do pruning after main loop of MAS",
                            "true");

    dominance::TauLabelManager::add_options_to_parser(parser);
    parser.add_option<shared_ptr<dominance::DominanceFunctionBuilder>> ("analysis", "Method to perform dominance analysis", "qld_simulation");

    options::Options opts = parser.parse();
    //auto cost_type = OperatorCost(opts.get_enum("cost_type"));

    bool task_has_zero_cost = g_main_task->get_min_operator_cost() == 0;

    if (parser.dry_run()) {
        return nullptr;
    } else {
        if (task_has_zero_cost) {
            return make_shared<NumericDominanceFTSPruning<IntEpsilon>>(opts);
        } else {
            return make_shared<NumericDominanceFTSPruning<int>>(opts);
        }
    }


}

static PluginTypePlugin<FTSTransitionPruning> _type_plugin("FTSTransitionPruning",
                                                           "Pruning of transitions in FTS");

static PluginShared<FTSTransitionPruning> _plugin("num_dominance_fts", _parse);


