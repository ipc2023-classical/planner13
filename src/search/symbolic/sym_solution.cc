#include "sym_solution.h"
#include <vector>       // std::vector
#include "../state_registry.h"
#include "../utils/debug_macros.h"
#include "unidirectional_search.h"
#include "../task_representation/search_task.h"

using namespace std;

namespace symbolic {
    void SymSolution::getPlan(vector<PlanState>& states, vector<OperatorID>& path) const { // Use OperatorID
        assert (path.empty()); //This code should be modified to allow appending things to paths
        DEBUG_MSG(cout << "Extract path forward: " << g << endl;);
        if (exp_fw) {
            exp_fw->getPlan(cut, g, path);
        }
        if (exp_bw) {
            DEBUG_MSG(cout << "Extract path backward: " << h << endl;);
            BDD newCut;
            if (!path.empty()) {
                // get correct state after forward search
                vector<int> s_i = task->get_initial_state();
                vector<PlanState> ss;
                ss.emplace_back(task->get_initial_state());
                for (OperatorID op : path) {
                    vector<int> s_n = s_i;
                    task->get_search_task()->apply_operator(s_i, op, s_n);
                    s_i = s_n;
                    ss.emplace_back(move(s_n));
                }
                
                newCut = exp_bw->getStateSpace()->getVars()->getStateBDD(ss.back().get_values());
            } else {
                newCut = cut;
            }

            exp_bw->getPlan(newCut, h, path);
        }

        // set states
        if (!path.empty()) {
            vector<int> s_i = task->get_initial_state();
            states.emplace_back(task->get_initial_state());
            for (OperatorID op : path) {
                vector<int> s_n = s_i;
                task->get_search_task()->apply_operator(s_i, op, s_n);
                s_i = s_n;
                states.emplace_back(move(s_n));
            }
        }
        /*DEBUG_MSG(cout << "Path extracted" << endl;
          State s2 (*g_initial_state);
          //Get state
          for(auto op : path){
          cout << op->get_name() << endl;
          if(!op->is_applicable(s2)){
          cout << "ERROR: bad plan reconstruction" << endl;
          cout << op->get_name() << " is not applicable" << endl;
          exit(-1);
          }
          s2 = State(s2, *op);
          }
          if(!test_goal(s2)){
          cout << "ERROR: bad plan reconstruction" << endl;
          cout << "The plan ends on a non-goal state" << endl;
          exit(-1);
          });*/
    }
    // --v Never called so its commented out for now
//    ADD SymSolution::getADD() const {
//        assert(exp_fw || exp_bw);
//        vector<const GlobalOperator *> path;
//        getPlan(path);
//
//        SymVariables *vars = nullptr;
//        if (exp_fw) vars = exp_fw->getStateSpace()->getVars();
//        else if (exp_bw) vars = exp_bw->getStateSpace()->getVars();
//
//        ADD hADD = vars->getADD(-1);
//        int h_val = g + h;
//
//        vector<int> s = g_initial_state_data;
//        BDD sBDD = vars->getStateBDD(s);
//        hADD += sBDD.Add() * (vars->getADD(h_val + 1));
//        for (auto op : path) {
//            h_val -= op->get_cost();
//            for (const GlobalEffect &eff : op->get_effects()) {
//                if (eff.does_fire(s)) {
//                    s[eff.var] = eff.val;
//                }
//            }
//            sBDD = vars->getStateBDD(s);
//            hADD += sBDD.Add() * (vars->getADD(h_val + 1));
//        }
//        return hADD;
//    }
}
