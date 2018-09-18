#include "sas_operator.h"

#include "../globals.h"

#include "../utils/collections.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;
using utils::ExitCode;

namespace task_representation {
void check_magic(istream &in, string magic) {
    string word;
    in >> word;
    if (word != magic) {
        cout << "Failed to match magic word '" << magic << "'." << endl;
        cout << "Got '" << word << "'." << endl;
        if (magic == "begin_version") {
            cerr << "Possible cause: you are running the planner "
                 << "on a preprocessor file from " << endl
                 << "an older version." << endl;
        }
        utils::exit_with(utils::ExitCode::INPUT_ERROR);
    }
}

SASCondition::SASCondition(istream &in) {
    in >> var >> val;
    g_sas_task()->check_fact(var, val);
}

SASCondition::SASCondition(int variable, int value)
    : var(variable),
      val(value) {
    g_sas_task()->check_fact(var, val);
}

// TODO if the input file format has been changed, we would need something like this
// Effect::Effect(istream &in) {
//    int cond_count;
//    in >> cond_count;
//    for (int i = 0; i < cond_count; ++i)
//        cond.push_back(Condition(in));
//    in >> var >> post;
//}

SASEffect::SASEffect(int variable, int value, const vector<SASCondition> &conds)
    : var(variable),
      val(value),
      conditions(conds) {
    g_sas_task()->check_fact(var, val);
}

void SASOperator::read_pre_post(istream &in) {
    int cond_count, var, pre, post;
    in >> cond_count;
    vector<SASCondition> conditions;
    conditions.reserve(cond_count);
    for (int i = 0; i < cond_count; ++i)
        conditions.push_back(SASCondition(in));
    in >> var >> pre >> post;
    if (pre != -1) {
        g_sas_task()->check_fact(var, pre);}
    g_sas_task()->check_fact(var, post);
    if (pre != -1) {
        preconditions.push_back(SASCondition(var, pre));
    }
    effects.push_back(SASEffect(var, post, conditions));
}

SASOperator::SASOperator(istream &in, bool axiom,
                         bool g_use_metric, int & g_min_action_cost, int & g_max_action_cost ) {
    is_an_axiom = axiom;
    if (!is_an_axiom) {
        check_magic(in, "begin_operator");
        in >> ws;
        getline(in, name);
        int count;
        in >> count;
        for (int i = 0; i < count; ++i)
            preconditions.push_back(SASCondition(in));
        in >> count;
        for (int i = 0; i < count; ++i)
            read_pre_post(in);

        int op_cost;
        in >> op_cost;
        cost = g_use_metric ? op_cost : 1;

        g_min_action_cost = min(g_min_action_cost, cost);
        g_max_action_cost = max(g_max_action_cost, cost);

        check_magic(in, "end_operator");
    } else {
        name = "<axiom>";
        cost = 0;
        check_magic(in, "begin_rule");
        read_pre_post(in);
        check_magic(in, "end_rule");
    }
}

void SASCondition::dump() const {
    cout << /*g_variable_name[var]*/ "var" << var << ": " << val;
}

void SASEffect::dump() const {
    cout << /*g_variable_name[var]*/ "var" <<var << ":= " << val;
    if (!conditions.empty()) {
        cout << " if";
        for (size_t i = 0; i < conditions.size(); ++i) {
            cout << " ";
            conditions[i].dump();
        }
    }
}

void SASOperator::dump() const {
    cout << name << ":";
    for (size_t i = 0; i < preconditions.size(); ++i) {
        cout << " [";
        preconditions[i].dump();
        cout << "]";
    }
    for (size_t i = 0; i < effects.size(); ++i) {
        cout << " [";
        effects[i].dump();
        cout << "]";
    }
    cout << endl;
}

// int get_op_index_hacked(const SASOperator *op) {
//     int op_index = op - &*g_operators.begin();
//     assert(op_index >= 0 && op_index < static_cast<int>(g_operators.size()));
//     return op_index;
// }
}
