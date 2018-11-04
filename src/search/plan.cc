#include "plan.h"

#include <iostream>
#include <fstream>

void Plan::set_plan(std::vector<GlobalState> states, std::vector<int> operators) {
    states = states;
    labels = operators;
    solved=true;
}
