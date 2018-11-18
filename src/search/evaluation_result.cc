#include "evaluation_result.h"

using namespace std;

const int EvaluationResult::INFTY = numeric_limits<int>::max();

EvaluationResult::EvaluationResult() : h_value(UNINITIALIZED) {
}

bool EvaluationResult::is_uninitialized() const {
    return h_value == UNINITIALIZED;
}

bool EvaluationResult::is_infinite() const {
    return h_value == INFTY;
}

int EvaluationResult::get_h_value() const {
    return h_value;
}

void EvaluationResult::get_preferred_operators(const GlobalState & state, const task_representation::SearchTask & search_task,
                                               const std::vector<OperatorID> & applicable_operators,
                                               ordered_set::OrderedSet<OperatorID> & result_preferred_operators) const {
    preferred_operators.get_preferred_operators(state, search_task,
                                                applicable_operators, result_preferred_operators);
}

bool EvaluationResult::get_count_evaluation() const {
    return count_evaluation;
}

void EvaluationResult::set_h_value(int value) {
    h_value = value;
}

void EvaluationResult::set_preferred_operators(
    PreferredOperatorsInfo &&preferred_ops) {
    preferred_operators = move(preferred_ops);
}

void EvaluationResult::set_count_evaluation(bool count_eval) {
    count_evaluation = count_eval;
}
