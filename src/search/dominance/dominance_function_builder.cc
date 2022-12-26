#include "dominance_function_builder.h"

#include "tau_labels.h"
#include "../task_transformation/factored_transition_system.h"
#include "dominance_function.h"

using namespace std;

namespace dominance {

    DominanceFunctionBuilder::DominanceFunctionBuilder(const Options &opts) :
            truncate_value(opts.get<int>("truncate_value")),
            max_simulation_time(opts.get<int>("max_simulation_time")),
            min_simulation_time(opts.get<int>("min_simulation_time")),
            max_total_time(opts.get<int>("max_total_time")),
            max_lts_size_to_compute_simulation(opts.get<int>("max_lts_size_to_compute_simulation")),
            num_labels_to_use_dominates_in(opts.get<int>("num_labels_to_use_dominates_in")),
            dump(opts.get<bool>("dump")),
            tau_label_manager(make_shared<TauLabelManager>(opts)) {
    }

    void DominanceFunctionBuilder::dump_options() const {
        cout << "truncate_value: " << truncate_value <<
             "\n num_labels_to_use_dominates_in: " << num_labels_to_use_dominates_in <<
             "\n max_lts_size_to_compute_simulation: " << max_lts_size_to_compute_simulation <<
             "\n max_simulation_time: " << max_simulation_time <<
             "\n min_simulation_time: " << min_simulation_time <<
             "\n max_total_time: " << max_total_time << '\n';
    }

    template<typename TCost>
    shared_ptr <DominanceFunction<TCost>>
    DominanceFunctionBuilder::compute_dominance_function(const FTSTask &fts_task, bool only_reachability) const {
        const auto &tss = fts_task.get_transition_systems();
        const auto &labels = fts_task.get_labels();

        return compute_dominance_function<TCost>(tss, labels, only_reachability);
    }

    template shared_ptr<DominanceFunction<int>> DominanceFunctionBuilder::compute_dominance_function<int>(const FTSTask &fts_task, bool only_reachability) const;
    template shared_ptr<DominanceFunction<IntEpsilon>> DominanceFunctionBuilder::compute_dominance_function<IntEpsilon>(const FTSTask &fts_task, bool only_reachability) const;
        template<typename TCost>
    shared_ptr <DominanceFunction<TCost>>
    DominanceFunctionBuilder::compute_dominance_function(const FactoredTransitionSystem &fts_task,
                                                         bool only_reachability) const {
        const auto &tss = fts_task.get_transition_systems();
        const auto &labels = fts_task.get_labels();

        return compute_dominance_function<TCost>(tss, labels, only_reachability);
    }
    template shared_ptr<DominanceFunction<int>> DominanceFunctionBuilder::compute_dominance_function<int>(const FactoredTransitionSystem &fts_task, bool only_reachability) const;
    template shared_ptr<DominanceFunction<IntEpsilon>> DominanceFunctionBuilder::compute_dominance_function<IntEpsilon>(const FactoredTransitionSystem &fts_task, bool only_reachability) const;


    template<typename TCost>
    shared_ptr <DominanceFunction<TCost>>
    DominanceFunctionBuilder::compute_dominance_function(const std::vector<std::unique_ptr<TransitionSystem>> &tss,
                                                         const Labels &labels, bool only_reachability) const {

        std::vector<std::unique_ptr<LocalDominanceFunction<TCost>>> local_functions;
        LabelDominanceFunction<TCost> label_relation (labels);

        auto tau_labels = tau_label_manager->compute_tau_labels<TCost>(tss, labels, !only_reachability);

        size_t num_ts = tss.size();
        for (size_t ts_id = 0; ts_id < num_ts; ++ts_id) {
            assert (tss[ts_id]);
            auto res = std::make_unique<LocalDominanceFunction<TCost>>(*(tss[ts_id]), ts_id, truncate_value, tau_labels);
            res->init_goal_respecting();
            local_functions.push_back(std::move(res));
        }

        utils::Timer t;
        int num_iterations = 0;
        int num_inner_iterations = 0;

        if (dump) {
            cout << "Compute numLDSim on " << num_ts << " TSs.\n"
                 << "Compute tau labels\n";
        }

        label_relation.init(tss, local_functions, labels.get_size() <= num_labels_to_use_dominates_in);
        for (size_t i = 0; i < local_functions.size(); ++i) {
            if (tss[i]->get_size() > max_lts_size_to_compute_simulation) {
                if (dump) {
                    cout << "Computation of numeric simulation on LTS " << i <<
                         " with " << tss[i]->get_size() << " states cancelled because it is too big.\n";
                }
                local_functions[i]->cancel_simulation_computation();
            }
        }

        vector<int> order_by_size;
        for (int i = 0; i < int(local_functions.size()); i++) {
            order_by_size.push_back(i);
        }

        sort(order_by_size.begin(), order_by_size.end(), [&](int a, int b) {
            return tss[a]->get_size() < tss[b]->get_size();
        });
        cout << "  Init numLDSim in " << t() << "s: " << flush;
        bool restart;
        do {
            do {
                num_iterations++;
                //label_relation.dump();
                int remaining_to_compute = int(order_by_size.size());
                for (int i: order_by_size) {
                    /* cout << "Updating " << i << " of size " <<   _ltss[i]->size() << " states and " */
                    /*           <<  _ltss[i]->num_transitions() << " transitions\n"; */

                    int max_time = max(max_simulation_time,
                                       min(min_simulation_time,
                                           1 + max_total_time / remaining_to_compute--));
                    num_inner_iterations += local_functions[i]->update(label_relation, max_time);
                    //_dominance_relation[i]->dump(_ltss[i]->get_names());
                }
                cout << " " << t() << "s" << flush;
            } while (label_relation.update(tss, local_functions));
            restart = tau_labels->add_noop_dominance_tau_labels(tss, label_relation);
            if (restart) {
                for (int i: order_by_size) {
                    local_functions[i]->init_goal_respecting();
                }
            }
        } while (restart);
        cout << "\nNumeric LDSim computed " << t() << "\n";
        cout << "Numeric LDSim outer iterations: " << num_iterations << "\n";
        cout << "Numeric LDSim inner iterations: " << num_inner_iterations << "\n";

        if (dump) {
            cout << "" << "------" << "\n";
            for (int i = 0; i < int(tss.size()); i++) {
                local_functions[i]->statistics();
                cout << "------" << "\n";
            }

            cout << "------" << "\n";
            for (int i = 0; i < int(tss.size()); i++) {
                //local_functions[i]->dump(tss[i]->get_names());
                cout << "------" << "\n";
                label_relation.dump(*(tss[i]), i);
            }
            //label_relation.dump_equivalent();
            //label_relation.dump_dominance();
            //exit(0);
        }


/*            ndr->total_max_value = 0;
            for (auto &sim: ndr->simulations) {
                ndr->total_max_value += sim->compute_max_value();
            }
*/
        return make_shared<DominanceFunction<TCost>>(std::move(local_functions), label_relation);
    }
}

using namespace dominance;
static shared_ptr<DominanceFunctionBuilder> _parse_num_dominance(options::OptionParser &parser) {
    parser.document_synopsis("Dominance pruning method", "");

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

    parser.add_option<bool>("dump",
                           "Prints out debug info",
                           "true");


    TauLabelManager::add_options_to_parser(parser);

    Options opts = parser.parse();
    //auto cost_type = OperatorCost(opts.get_enum("cost_type"));

    if (parser.dry_run()) {
        return nullptr;
    } else {
        return make_shared<DominanceFunctionBuilder>(opts);
    }
}

static PluginShared<DominanceFunctionBuilder> _plugin("qld_simulation", _parse_num_dominance);

static PluginTypePlugin<DominanceFunctionBuilder> _type_plugin(
       "DominanceFunctionBuilder",
       "Constructs a dominance function for a planning task.");

