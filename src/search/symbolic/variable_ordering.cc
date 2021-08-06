#include "../task_representation/transition_system.h"
#include "../options/options.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../utils/rng_options.h"
#include "variable_ordering.h"
#include "opt_order.h"



using namespace std;
using options::Options;

namespace symbolic {

    InputVariableOrdering::InputVariableOrdering(Options &) {
        name = "default variable ordering";
    }

    void InputVariableOrdering::compute_variable_ordering(vector<int> &var_order, const shared_ptr<task_representation::FTSTask>& task) {
        assert(var_order.empty());
        var_order.reserve(task->get_size());
        for (size_t i = 0; i < size_t(task->get_size()); ++i) {
            var_order.emplace_back(i);
        }
    }

    GamerVariableOrdering::GamerVariableOrdering(Options &opts) :
            random_seed(opts.get<int>("random_seed")),
            rng(utils::parse_rng_from_options(opts)) {
        name = "Gamer variable ordering";
    }

    void GamerVariableOrdering::compute_variable_ordering(vector<int> &var_order, const shared_ptr<task_representation::FTSTask>& task) {
        InfluenceGraph::compute_gamer_ordering(var_order, task, rng);
    }


    // Plugin
    static shared_ptr<InputVariableOrdering> _parse_default(options::OptionParser &parser) {
        parser.document_synopsis(
                "Default variable reordering. Does nothing, basically.",
                "Variable reordering");

        options::Options opts = parser.parse();
        if (parser.dry_run()) {
            return nullptr;
        }

        return std::make_shared<InputVariableOrdering>(opts);
    }

    static PluginShared<VariableOrdering> _plugin_default("input", _parse_default);


    static shared_ptr<GamerVariableOrdering> _parse_gamer(options::OptionParser &parser) {
        parser.document_synopsis(
                "Gamer variable ordering",
                "Variable ordering for symbolic BDDS");

        // Add random_seed option.
        utils::add_rng_options(parser);

        options::Options opts = parser.parse();
        if (parser.dry_run()) {
            return nullptr;
        }

        return std::make_shared<GamerVariableOrdering>(opts);
    }

    static PluginShared<VariableOrdering> _plugin_gamer("gamer", _parse_gamer);

    static PluginTypePlugin<VariableOrdering> _type_plugin("variable_ordering", "This describes the strategy for computing variable ordering for the BDD construction. ");
}