#include "shrink_strategy.h"

#include "../plugin.h"

#include <iostream>

using namespace std;

namespace task_transformation {
void ShrinkStrategy::dump_options() const {
    cout << "Shrink strategy options: " << endl;
    cout << "Type: " << name() << endl;
    dump_strategy_specific_options();
}

string ShrinkStrategy::get_name() const {
    return name();
}

static PluginTypePlugin<ShrinkStrategy> _type_plugin(
    "ShrinkStrategy",
    "This page describes the various shrink strategies supported "
    "by the planner.");
}
