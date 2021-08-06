#include "sym_variables.h"

#include "../options/options.h"
#include "../utils/rng_options.h"
#include "state_reordering.h"
#include "../task_representation/transition_system.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

using namespace std;
using options::Options;

namespace symbolic {
    symbolic::DefaultStateReordering::DefaultStateReordering(const options::Options&) {
        name = "default state reordering";
    }

    void DefaultStateReordering::computeStateReordering(std::vector<int> &var_order,
                                                        std::map<int, std::vector<int>> &var_to_state,
                                                        const std::shared_ptr<task_representation::FTSTask> &task) {
        for (auto var : var_order) {
            vector<int> s = vector<int>();
            s.reserve(task->get_ts(var).get_size());
            for (int i = 0; i < task->get_ts(var).get_size(); i++) {
                s.emplace_back(i);
            }

            var_to_state[var] = s;
        }
    }

    symbolic::RandomStateReordering::RandomStateReordering(const options::Options& options) :
            random_seed(options.get<int>("random_seed")),
            rng(utils::parse_rng_from_options(options)) {
        name = "random state reordering";
    }

    void RandomStateReordering::computeStateReordering(std::vector<int> &var_order,
                                                       std::map<int, std::vector<int>>& var_to_state,
                                                       const std::shared_ptr<task_representation::FTSTask> &task) {

        // fill the vectors
        for (auto var : var_order) {
            vector<int> s = vector<int>();
            s.reserve(task->get_ts(var).get_size());
            for (int i = 0; i < task->get_ts(var).get_size(); i++) {
                s.emplace_back(i);
            }

            var_to_state[var] = s;
        }

        // shuffle the vectors
        for (auto var : var_order) {
            rng->shuffle(var_to_state[var]);
        }
    }

    // Plugin
    static shared_ptr<DefaultStateReordering> _parse_default(options::OptionParser &parser) {
        parser.document_synopsis(
                "Default state reordering. Does nothing",
                "State reordering");

        options::Options opts = parser.parse();
        if (parser.dry_run()) {
            return nullptr;
        }

        return std::make_shared<DefaultStateReordering>(opts);
    }

    static PluginShared<StateReordering> _plugin_default("default", _parse_default);


    static shared_ptr<RandomStateReordering> _parse_gamer(options::OptionParser &parser) {
        parser.document_synopsis(
                "Random state reordering. Does nothing",
                "State reordering");

        // Add random_seed option.
        utils::add_rng_options(parser);

        options::Options opts = parser.parse();
        if (parser.dry_run()) {
            return nullptr;
        }

        return std::make_shared<RandomStateReordering>(opts);
    }

    static PluginShared<StateReordering> _plugin_random("random", _parse_gamer);

    static PluginTypePlugin<StateReordering> _type_plugin("state_reordering", "This describes the strategy for computing state ordering for the BDD construction. ");
}