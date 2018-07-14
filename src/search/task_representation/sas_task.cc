#include "sas_task.h"

#include <iostream>
#include <limits>

#include "sas_operator.h"
#include "../utils/system.h"
#include "../utils/timer.h"
#include "fact.h"

using namespace std;

const FactPair FactPair::no_fact = FactPair(-1, -1);

ostream &operator<<(ostream &os, const FactPair &fact_pair) {
    os << fact_pair.var << "=" << fact_pair.value;
    return os;
}

void SASTask::read_metric(istream &in) {
    utils::check_magic(in, "begin_metric");
    in >> g_use_metric;
    utils::check_magic(in, "end_metric");
}

void SASTask::read_variables(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; ++i) {
        utils::check_magic(in, "begin_variable");
        string name;
        in >> name;
        g_variable_name.push_back(name);
        int layer;
        in >> layer;
        g_axiom_layers.push_back(layer);
        int range;
        in >> range;
        g_variable_domain.push_back(range);
        in >> ws;
        vector<string> fact_names(range);
        for (size_t j = 0; j < fact_names.size(); ++j)
            getline(in, fact_names[j]);
        g_fact_names.push_back(fact_names);
        utils::check_magic(in, "end_variable");
    }
}

void SASTask::read_mutexes(istream &in) {
    g_inconsistent_facts.resize(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        g_inconsistent_facts[i].resize(g_variable_domain[i]);

    int num_mutex_groups;
    in >> num_mutex_groups;

    /* NOTE: Mutex groups can overlap, in which case the same mutex
       should not be represented multiple times. The current
       representation takes care of that automatically by using sets.
       If we ever change this representation, this is something to be
       aware of. */

    for (int i = 0; i < num_mutex_groups; ++i) {
        utils::check_magic(in, "begin_mutex_group");
        int num_facts;
        in >> num_facts;
        vector<FactPair> invariant_group;
        invariant_group.reserve(num_facts);
        for (int j = 0; j < num_facts; ++j) {
            int var;
            int value;
            in >> var >> value;
            invariant_group.emplace_back(var, value);
        }
        utils::check_magic(in, "end_mutex_group");
        for (const FactPair &fact1 : invariant_group) {
            for (const FactPair &fact2 : invariant_group) {
                if (fact1.var != fact2.var) {
                    /* The "different variable" test makes sure we
                       don't mark a fact as mutex with itself
                       (important for correctness) and don't include
                       redundant mutexes (important to conserve
                       memory). Note that the translator (at least
                       with default settings) removes mutex groups
                       that contain *only* redundant mutexes, but it
                       can of course generate mutex groups which lead
                       to *some* redundant mutexes, where some but not
                       all facts talk about the same variable. */
                    g_inconsistent_facts[fact1.var][fact1.value].insert(fact2);
                }
            }
        }
    }
}

void SASTask::read_goal(istream &in) {
    utils::check_magic(in, "begin_goal");
    int count;
    in >> count;
    if (count < 1) {
        cerr << "Task has no goal condition!" << endl;
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
    for (int i = 0; i < count; ++i) {
        int var, val;
        in >> var >> val;
        g_goal.push_back(make_pair(var, val));
    }
    utils::check_magic(in, "end_goal");
}

void SASTask::read_operators(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; ++i)
        g_operators.push_back(SASOperator(in, false,
					     g_use_metric, g_min_action_cost, g_max_action_cost));
}

void SASTask::read_axioms(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; ++i)
        g_axioms.push_back(SASOperator(in, true,
					  g_use_metric, g_min_action_cost, g_max_action_cost));

    // g_axiom_evaluator = new AxiomEvaluator(TaskProxy(*g_root_task()));
}

SASTask::SASTask() : g_min_action_cost (numeric_limits<int>::max()),
				     g_max_action_cost (0) {

}
void SASTask::read_from_file(istream &in)  {
    cout << "reading input... [t=" << utils::g_timer << "]" << endl;
    utils::read_and_verify_version(in);
    
    read_metric(in);
    read_variables(in);
    read_mutexes(in);
    g_initial_state_data.resize(g_variable_domain.size());
    utils::check_magic(in, "begin_state");
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        in >> g_initial_state_data[i];
    }
    utils::check_magic(in, "end_state");
    g_default_axiom_values = g_initial_state_data;

    read_goal(in);
    read_operators(in);
    read_axioms(in);

    /* TODO: We should be stricter here and verify that we
       have reached the end of "in". */

    cout << "done reading input! [t=" << utils::g_timer << "]" << endl;

    // cout << "packing state variables..." << flush;
    // assert(!g_variable_domain.empty());
    // g_state_packer = new int_packer::IntPacker(g_variable_domain);
    // cout << "done! [t=" << utils::g_timer << "]" << endl;

    int num_vars = g_variable_domain.size();
    int num_facts = 0;
    for (int var = 0; var < num_vars; ++var)
        num_facts += g_variable_domain[var];

    cout << "Variables: " << num_vars << endl;
    cout << "FactPairs: " << num_facts << endl;
    // cout << "Bytes per state: "
    //      << g_state_packer->get_num_bins() * sizeof(int_packer::IntPacker::Bin)
    //      << endl;
}


bool SASTask::test_goal(const GlobalState &state) const {
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (state[g_goal[i].first] != g_goal[i].second) {
            return false;
        }
    }
    return true;
}


const SASOperator &SASTask::get_operator_or_axiom(int index, bool is_axiom) const {
    if (is_axiom) {
        assert(utils::in_bounds(index, g_axioms));
        return g_axioms[index];
    } else {
        assert(utils::in_bounds(index, g_operators));
        return g_operators[index];
    }
}


int SASTask::get_num_variables() const {
    return g_variable_domain.size();
}

string SASTask::get_variable_name(int var) const {
    return g_variable_name[var];
}

int SASTask::get_variable_domain_size(int var) const {
    return g_variable_domain[var];
}

int SASTask::get_variable_axiom_layer(int var) const {
    return g_axiom_layers[var];
}

int SASTask::get_variable_default_axiom_value(int var) const {
    return g_default_axiom_values[var];
}

string SASTask::get_fact_name(const FactPair &fact) const {
    return g_fact_names[fact.var][fact.value];
}


int SASTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_cost();
}

string SASTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_name();
}

int SASTask::get_num_operators() const {
    return g_operators.size();
}

int SASTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_preconditions().size();
}

FactPair SASTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const SASOperator &op = get_operator_or_axiom(op_index, is_axiom);
    const SASCondition &precondition = op.get_preconditions()[fact_index];
    return FactPair(precondition.var, precondition.val);
}

int SASTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects().size();
}

int SASTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index].conditions.size();
}

FactPair SASTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const SASEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    const SASCondition &condition = effect.conditions[cond_index];
    return FactPair(condition.var, condition.val);
}

FactPair SASTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    const SASEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    return FactPair(effect.var, effect.val);
}

OperatorID SASTask::get_global_operator_id(OperatorID id) const {
    return id;
}

int SASTask::get_num_axioms() const {
    return g_axioms.size();
}

int SASTask::get_num_goals() const {
    return g_goal.size();
}

FactPair SASTask::get_goal_fact(int index) const {
    const pair<int, int> &goal = g_goal[index];
    return FactPair(goal.first, goal.second);
}

void SASTask::convert_state_values(
    vector<int> &, const SASTask *ancestor_task) const {
    if (this != ancestor_task) {
        ABORT("Invalid state conversion");
    }
}


void SASTask::dump_goal() const {
    cout << "Goal Conditions:" << endl;
    for (size_t i = 0; i < g_goal.size(); ++i)
        cout << "  " << g_variable_name[g_goal[i].first] << ": "
             << g_goal[i].second << endl;
}

void SASTask::dump_everything() const {
    cout << "Use metric? " << g_use_metric << endl;
    cout << "Min Action Cost: " << g_min_action_cost << endl;
    cout << "Max Action Cost: " << g_max_action_cost << endl;
    // TODO: Dump the actual fact names.
    cout << "Variables (" << g_variable_name.size() << "):" << endl;
    for (size_t i = 0; i < g_variable_name.size(); ++i)
        cout << "  " << g_variable_name[i]
             << " (range " << g_variable_domain[i] << ")" << endl;
    // cout << "Initial State (PDDL):" << endl;
    // initial_state.dump_pddl();
    // cout << "Initial State (FDR):" << endl;
    // initial_state.dump_fdr();
    dump_goal();
    /*
    for(int i = 0; i < g_variable_domain.size(); ++i)
      g_transition_graphs[i]->dump();
    */
}

bool SASTask::is_unit_cost()  const {
    return g_min_action_cost == 1 && g_max_action_cost == 1;
}

bool SASTask::has_axioms() const {
    return !g_axioms.empty();
}

void SASTask::verify_no_axioms() const {
    if (has_axioms()) {
        cerr << "Heuristic does not support axioms!" << endl << "Terminating."
             << endl;
        utils::exit_with(utils::ExitCode::UNSUPPORTED);
    }
}

int SASTask::get_first_conditional_effects_op_id() const {
    for (size_t i = 0; i < g_operators.size(); ++i) {
        const vector<SASEffect> &effects = g_operators[i].get_effects();
        for (size_t j = 0; j < effects.size(); ++j) {
            const vector<SASCondition> &cond = effects[j].conditions;
            if (!cond.empty())
                return i;
        }
    }
    return -1;
}

bool SASTask::has_conditional_effects() const {
    return get_first_conditional_effects_op_id() != -1;
}

void SASTask::verify_no_conditional_effects() const {
    int op_id = get_first_conditional_effects_op_id();
    if (op_id != -1) {
        cerr << "Heuristic does not support conditional effects "
             << "(operator " << g_operators[op_id].get_name() << ")" << endl
             << "Terminating." << endl;
        utils::exit_with(utils::ExitCode::UNSUPPORTED);
    }
}

void SASTask::verify_no_axioms_no_conditional_effects() const {
    verify_no_axioms();
    verify_no_conditional_effects();
}

bool SASTask::are_facts_mutex(const FactPair &a, const FactPair &b) const {
    if (a.var == b.var) {
        // Same variable: mutex iff different value.
        return a.value != b.value;
    }
    return bool(g_inconsistent_facts[a.var][a.value].count(b));
}

void SASTask::check_fact(int var, int val) const {
    if (!utils::in_bounds(var, g_variable_domain)) {
        cerr << "Invalid variable id: " << var << endl;
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
    if (val < 0 || val >= g_variable_domain[var]) {
        cerr << "Invalid value for variable " << var << ": " << val << endl;
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
}

