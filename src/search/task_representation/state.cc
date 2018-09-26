#include "state.h"

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
    cerr <<  "State::dump_pddl not implemented" << endl;
    utils::exit_with(utils::ExitCode::UNSUPPORTED);
}

void State::dump_fdr() const {
    cerr <<  "State::dump_fdr not implemented" << endl;
    utils::exit_with(utils::ExitCode::UNSUPPORTED);
}
}
