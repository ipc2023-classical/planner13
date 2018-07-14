#include "globals.h"

#include "axioms.h"
#include "global_state.h"
#include "heuristic.h"

#include "algorithms/int_packer.h"
#include "utils/logging.h"
#include "utils/rng.h"
#include "utils/system.h"
#include "utils/timer.h"

#include "task_representation/state.h"
#include "task_representation/sas_task.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using utils::ExitCode;


int calculate_plan_cost(const vector<int> &plan) {
    // TODO: Refactor: this is only used by save_plan (see below) and the SearchEngine
    //       classes and hence should maybe be moved into the SearchEngine (along with
    //       save_plan).
    int plan_cost = 0;
    for (int op : plan) {
        plan_cost += g_sas_task->get_operator_cost(op);
    }
    return plan_cost;
}

void save_plan(const vector<int> &plan,
               bool generates_multiple_plan_files) {
    // TODO: Refactor: this is only used by the SearchEngine classes
    //       and hence should maybe be moved into the SearchEngine.
    ostringstream filename;
    filename << g_plan_filename;
    int plan_number = g_num_previously_generated_plans + 1;
    if (generates_multiple_plan_files || g_is_part_of_anytime_portfolio) {
        filename << "." << plan_number;
    } else {
        assert(plan_number == 1);
    }
    ofstream outfile(filename.str());
    for (size_t i = 0; i < plan.size(); ++i) {
        cout << g_sas_task->get_operator_name(plan[i]) << " (" << g_sas_task->get_operator_cost(plan[i]) << ")" << endl;
        outfile << "(" << g_sas_task->get_operator_name(plan[i]) << ")" << endl;
    }
    int plan_cost = calculate_plan_cost(plan);
    outfile << "; cost = " << plan_cost << " ("
            << (g_sas_task->is_unit_cost() ? "unit cost" : "general cost") << ")" << endl;
    outfile.close();
    cout << "Plan length: " << plan.size() << " step(s)." << endl;
    cout << "Plan cost: " << plan_cost << endl;
    ++g_num_previously_generated_plans;
}

// AxiomEvaluator *g_axiom_evaluator;
// successor_generator::SuccessorGenerator *g_successor_generator;

string g_plan_filename = "sas_plan";
int g_num_previously_generated_plans = 0;
bool g_is_part_of_anytime_portfolio = false;


const std::shared_ptr<SASTask> g_sas_task = make_shared<SASTask>();
std::shared_ptr<FTSTask> g_main_task;

utils::Log g_log;
