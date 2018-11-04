#include "plan.h"

#include <iostream>
#include <fstream>
#include <cassert>

void Plan::set_plan(const std::vector<GlobalState> & states_, const std::vector<int>& operators_) {
    states = states_;
    labels = operators_;
    solved=true;
    assert(states.size() == labels.size() + 1);
}
