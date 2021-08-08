#include "numeric_dominance_fts_pruning.h"
#include "../task_transformation/factored_transition_system.h"
#include "../options/options.h"
#include "../options/option_parser.h"
#include "../plugin.h"
#include "../globals.h"

#include <set>
#include <memory>

using namespace numeric_dominance;
using namespace std;

template<typename T>
std::vector<int> NumericDominanceFTSPruning<T>::prune_transitions(FactoredTransitionSystem &fts) const {
    return prune_dominated_transitions(fts, compute_dominance_relation(fts));
}

template<typename T>
std::vector<int> NumericDominanceFTSPruning<T>::prune_dominated_transitions(FactoredTransitionSystem &fts,
                                                               std::shared_ptr<NumericDominanceRelation<T>> ndrel) const {
    assert(fts.get_num_active_entries() == ndrel->size());

    const Labels& labels = fts.get_labels();
    const NumericLabelRelation<T> label_relation = ndrel->get_label_relation();

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
            const TransitionSystem& ts_label_is_dominated_in = fts.get_ts(ts_id);
            if (label_relation.q_dominated_by_noop(l_id, ts_id) >= 0) {
                for (const Transition& tr : ts_label_is_dominated_in.get_transitions_with_label(l_id)) {
                    if ((*ndrel)[ts_id].simulates(tr.src, tr.target)
                    && ndrel->propagate_transition_pruning(ts_id, ts_label_is_dominated_in, tr.src, l_id, tr.target)) {

                        add_transition_to_be_removed(ts_id, l_id, tr);
                    }
                }
            }
        } else if (ts_id == DOMINATES_IN_ALL) {
            T dom = label_relation.q_dominated_by_noop(l_id);
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
        const TransitionSystem& ts = fts.get_ts(ts_id);

        for (LabelGroupID lg1_id(0); lg1_id < ts.num_label_groups(); ++lg1_id) {
            for (LabelGroupID lg2_id(0); lg2_id < ts.num_label_groups(); ++lg2_id) {
                for (const Transition &tr1 : ts.get_transitions_for_group_id(lg1_id)) {
                    const vector<Transition>& trs2 = ts.get_transitions_for_group_id(lg2_id);
                    auto tr2 = std::lower_bound(trs2.begin(), trs2.end(), Transition(tr1.src, 0));
                    auto tr2_end = std::upper_bound(tr2, trs2.end(), Transition(tr1.src, std::numeric_limits<int>::max()));

                    for (; tr2 != tr2_end; tr2++) {
                        assert(tr1.src == tr2->src);
                        for (int l1_id : ts.get_label_group(lg1_id)) {
                            for (int l2_id : ts.get_label_group(lg2_id)) {
                                if (l1_id == l2_id) continue;

                                // Handle special -infinity cases to avoid underflow
                                T l_qdom = label_relation.q_dominates(LabelID(l1_id), LabelID(l2_id), ts_id);
                                if (l_qdom == std::numeric_limits<int>::lowest()) continue;

                                T s_qsim = (*ndrel)[ts_id].q_simulates(tr1.target, tr2->target);
                                if (s_qsim == std::numeric_limits<int>::lowest()) continue;

                                // Check if src -- l1_id --> tr1.target dominates src -- l2_id --> tr2.target
                                if ( l_qdom + s_qsim - labels.get_label_cost(l1_id) + labels.get_label_cost(l2_id) >= 0) {

                                    if (ndrel->propagate_transition_pruning(ts_id, ts, tr2->src, LabelID(l1_id), tr2->target)) {
                                        std::cout << tr1.src << "--" << l1_id << "->" << tr1.target << " dominates " << tr2->src << "--" << l2_id << "->" << tr2->target << std::endl;

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

    std::cout << "Pruned " << num_pruned_transitions << " transitions and " << labels_to_remove.size() << " labels" << endl;

    vector<int> affected_ts_ids_vec;
    affected_ts_ids_vec.reserve(affected_ts_ids.size());
    affected_ts_ids_vec.insert(affected_ts_ids_vec.end(), affected_ts_ids.begin(),  affected_ts_ids.end());

    return affected_ts_ids_vec;
}

template<typename T>
shared_ptr<NumericDominanceRelation<T>> NumericDominanceFTSPruning<T>::compute_dominance_relation(const FactoredTransitionSystem &fts) const {

    vector<TransitionSystem> tss;
    fts.get_transition_systems(tss);

    auto numeric_dominance_relation_builder = NumericDominanceRelationBuilder<T>(
            tss,
            fts.get_labels(),
            truncate_value, max_simulation_time,
            min_simulation_time, max_total_time,
            max_lts_size_to_compute_simulation,
            num_labels_to_use_dominates_in,
            tau_labels);

    numeric_dominance_relation_builder.init();
    return numeric_dominance_relation_builder.compute_ld_simulation(dump);
}

template<typename T>
NumericDominanceFTSPruning<T>::NumericDominanceFTSPruning(options::Options opts) :
        truncate_value(opts.get<int>("truncate_value")),
        max_simulation_time(opts.get<int>("max_simulation_time")),
        min_simulation_time(opts.get<int>("min_simulation_time")),
        max_total_time(opts.get<int>("max_total_time")),
        max_lts_size_to_compute_simulation(opts.get<int>("max_lts_size_to_compute_simulation")),
        num_labels_to_use_dominates_in(opts.get<int>("num_labels_to_use_dominates_in")),
        tau_labels(make_shared<TauLabelManager<T>>(opts, false)),
        prune_transitions_before_main_loop(opts.get<bool>("prune_before")),
        prune_transitions_after_main_loop(opts.get<bool>("prune_after")),
        dump(opts.get<bool>("dump")) {

}


static shared_ptr<FTSTransitionPruning> _parse(options::OptionParser &parser) {
    parser.document_synopsis("Simulation heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional_effects", "supported (but see note)");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.document_note(
            "Note",
            "Conditional effects are supported directly. Note, however, that "
            "for tasks that are not factored (in the sense of the JACM 2014 "
            "merge-and-shrink paper), the atomic abstractions on which "
            "merge-and-shrink heuristics are based are nondeterministic, "
            "which can lead to poor heuristics even when no shrinking is "
            "performed.");


    parser.add_option<bool>("dump",
                            "Dumps the relation that has been found",
                            "false");

    parser.add_option<int>("truncate_value",
                           "Assume -infinity if below minus this value",
                           "1000");

    parser.add_option<int>("max_simulation_time",
                           "Maximum number of seconds spent in computing a single update of a simulation", "1800000");

    parser.add_option<int>("min_simulation_time",
                           "Minimum number of seconds spent in computing a single update of a simulation",
                           "100000"); // By default we do not have any limit

    parser.add_option<int>("max_total_time",
                           "Maximum number of seconds spent in computing all updates of a simulation", "1800000");

    parser.add_option<int>("max_lts_size_to_compute_simulation",
                           "Avoid computing simulation on ltss that have more states than this number",
                           "1000000");

    parser.add_option<int>("num_labels_to_use_dominates_in",
                           "Use _may_dominate_in for instances that have less than this amount of labels",
                           "0");

    parser.add_option<bool>("prune_before",
                            "Do pruning before main loop of MAS",
                            "true");

    parser.add_option<bool>("prune_after",
                            "Do pruning after main loop of MAS",
                            "true");

    TauLabelManager<int>::add_options_to_parser(parser);

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


template class numeric_dominance::NumericDominanceFTSPruning<int>;
template class numeric_dominance::NumericDominanceFTSPruning<IntEpsilon>;