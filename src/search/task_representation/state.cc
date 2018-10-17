#include "state.h"

#include "sas_task.h"

#include "../globals.h"

#include <iostream>

using namespace std;

namespace task_representation {


// State State::get_successor(const FTSOperator & op) const {
//     // if (task->has_axioms()) {
//     //         ABORT("State::apply currently does not support axioms.");
//     // }
//     assert(!op.is_axiom());
//     //assert(is_applicable(op, state));
//     std::vector<int> new_values = values;
//     for (const auto & effect_fact : op.get_effects()) {
//         //if (does_fire(effect, *this)) {
//         //Fact effect_fact = effect.get_fact();
//         new_values[effect_fact.var] = effect_fact.value;
//             //}
//     }
//     return State(*task, std::move(new_values));
// }


void State::dump_pddl() const {
    for (size_t var = 0; var < values.size(); ++var) {
        string fact_name = g_sas_task()->get_fact_name(FactPair(var, values[var]));
        if (fact_name != "<none of those>") {
            cout << fact_name << endl;
        }
    }
}

void State::dump_fdr() const {
    // The original dump_fdr method also printed the name of variables, which
    // we cannot do of course since we merge variables.
    for (size_t var = 0; var < values.size(); ++var) {
        cout << "  #" << var << " -> " << values[var] << endl;
    }
}
}
