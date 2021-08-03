#include "symbolic_search.h"


#include "../option_parser.h"
#include "../plugin.h"

#include "../symbolic/sym_search.h"
#include "../symbolic/sym_variables.h"
#include "../symbolic/sym_params_search.h"
#include "../symbolic/sym_state_space_manager.h"
#include "../symbolic/original_state_space.h"
#include "../symbolic/uniform_cost_search.h"
#include "../symbolic/bidirectional_search.h"
#include "../task_representation/search_task.h"
#include "../task_representation/sas_task.h"
#include "../task_representation/sas_operator.h"
#include "../task_representation/transition_system.h"



using namespace std;
using namespace symbolic;
using namespace options;


namespace symbolic_search {

    SymbolicSearch::SymbolicSearch(const options::Options &opts) :
            SearchEngine(opts), SymController(opts, g_main_task), task(g_main_task) { }

    SymbolicBidirectionalUniformCostSearch::SymbolicBidirectionalUniformCostSearch(const options::Options &opts) :
            SymbolicSearch(opts) {
    }

    void SymbolicBidirectionalUniformCostSearch::initialize() {
        mgr = make_shared<OriginalStateSpace>(vars.get(), mgrParams, task);
        auto fw_search = make_unique<UniformCostSearch>(this, searchParams, task);
        auto bw_search = make_unique<UniformCostSearch>(this, searchParams, task);
        fw_search->init(mgr, true, bw_search->getClosedShared());
        bw_search->init(mgr, false, fw_search->getClosedShared());

        search = make_unique<BidirectionalSearch>(this, searchParams, move(fw_search), move(bw_search));
    }


    SymbolicUniformCostSearch::SymbolicUniformCostSearch(const options::Options &opts, bool _fw) :
            SymbolicSearch(opts), fw(_fw) {
    }

    void SymbolicUniformCostSearch::initialize() {
        mgr = make_shared<OriginalStateSpace>(vars.get(), mgrParams, task);
        auto uni_search = make_unique<UniformCostSearch>(this, searchParams, task);
        if (fw) {
            uni_search->init(mgr, true, nullptr);
        } else {
            uni_search->init(mgr, false, nullptr);
        }

        search.reset(uni_search.release());

    }


    SearchStatus SymbolicSearch::step() {
        search->step();

        if (getLowerBound() < getUpperBound()) {
            return IN_PROGRESS;
        } else if (found_solution()) {

            return SOLVED;
        } else {
            return FAILED;
        }
    }

    void SymbolicSearch::new_solution(const SymSolution &sol) {
        if (sol.getCost() < getUpperBound()) {
            vector<PlanState> states;
            vector<OperatorID> plan;
            sol.getPlan(states, plan);
            this->check_goal_and_set_plan(states.back(), states, plan, task);
        }

        SymController::new_solution(sol);
    }
}

static shared_ptr<SearchEngine> _parse_bidirectional_ucs(OptionParser &parser) {
    parser.document_synopsis("Symbolic Bidirectional Uniform Cost Search", "");

    SearchEngine::add_options_to_parser(parser);
    SymVariables::add_options_to_parser(parser);
    SymParamsSearch::add_options_to_parser(parser, 30e3, 10e7);
    SymParamsMgr::add_options_to_parser(parser);

    Options opts = parser.parse();

    shared_ptr<symbolic_search::SymbolicSearch> engine;
    if (!parser.dry_run()) {
        engine = make_shared<symbolic_search::SymbolicBidirectionalUniformCostSearch>(opts);
    }

    return engine;
}

static shared_ptr<SearchEngine> _parse_forward_ucs(OptionParser &parser) {
    parser.document_synopsis("Symbolic Bidirectional Uniform Cost Search", "");

    SearchEngine::add_options_to_parser(parser);
    SymVariables::add_options_to_parser(parser);
    SymParamsSearch::add_options_to_parser(parser, 30e3, 10e7);
    SymParamsMgr::add_options_to_parser(parser);

    Options opts = parser.parse();

    shared_ptr<symbolic_search::SymbolicSearch> engine;
    if (!parser.dry_run()) {
        engine = make_shared<symbolic_search::SymbolicUniformCostSearch>(opts, true);
    }

    return engine;
}

static shared_ptr<SearchEngine> _parse_backward_ucs(OptionParser &parser) {
    parser.document_synopsis("Symbolic Bidirectional Uniform Cost Search", "");

    SearchEngine::add_options_to_parser(parser);
    SymVariables::add_options_to_parser(parser);
    SymParamsSearch::add_options_to_parser(parser, 30e3, 10e7);
    SymParamsMgr::add_options_to_parser(parser);

    Options opts = parser.parse();

    shared_ptr<symbolic_search::SymbolicSearch> engine;
    if (!parser.dry_run()) {
        engine = make_shared<symbolic_search::SymbolicUniformCostSearch>(opts, false);
    }

    return engine;
}


static PluginShared<SearchEngine> _plugin_bd("sbd", _parse_bidirectional_ucs);
static PluginShared<SearchEngine> _plugin_fw("sfw", _parse_forward_ucs);
static PluginShared<SearchEngine> _plugin_bw("sbw", _parse_backward_ucs);
