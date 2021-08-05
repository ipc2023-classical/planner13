#include "transition_relation.h"

#include <algorithm>
#include <cassert>
#include "../utils/debug_macros.h"
#include "../task_representation/transition_system.h"

#include "sym_state_space_manager.h"

using namespace std;

namespace symbolic {
TransitionRelation::TransitionRelation(SymVariables *sVars,
                                       const int label, const std::shared_ptr<task_representation::FTSTask> &_task) :
        sV(sVars), cost(_task->get_label_cost(label)), tBDD(sVars->oneBDD()),
        existsVars(sVars->oneBDD()), existsBwVars(sVars->oneBDD()),
        absAfterImage(nullptr), task(_task) {
    labels.insert(label);

    for (size_t i = 0; i < size_t(task->get_size()); i++) {
        task_representation::TransitionSystem ts = task->get_ts(int(i));
        const std::vector<task_representation::Transition> &transitions = ts.get_transitions_with_label(label);

        BDD BDD_label = sV->zeroBDD();
        for (auto transition : transitions) {
            BDD_label += sV->sourceStateBDD(int(i), transition.src) * sV->targetStateBDD(int(i), transition.target);
        }

        effVars.push_back(i); // TODO Future work: only include a subset of variables
        tBDD *= BDD_label;
    }

    assert(!tBDD.IsZero());

    sort(effVars.begin(), effVars.end());
    for (int var : effVars) {
        for (int bdd_var : sV->vars_index_pre(var)) {
            swapVarsS.push_back(sV->bddVar(bdd_var));
        }
        for (int bdd_var : sV->vars_index_eff(var)) {
            swapVarsSp.push_back(sV->bddVar(bdd_var));
        }
    }
    assert(swapVarsS.size() == swapVarsSp.size());
    // existsVars/existsBwVars is just the conjunction of swapVarsS and swapVarsSp
    for (size_t i = 0; i < swapVarsS.size(); ++i) {
        existsVars *= swapVarsS[i];
        existsBwVars *= swapVarsSp[i];
    }
//    DEBUG_MSG(cout << "Computing tr took " << tr_timer; tBDD.print(1, 1););
}

void TransitionRelation::shrink(const SymStateSpaceManager &abs, int maxNodes) {
    tBDD = abs.shrinkTBDD(tBDD, maxNodes);

    // effVars
    vector <int> newEffVars;
    for (int var : effVars) {
        if (abs.isRelevantVar(var)) {
            newEffVars.push_back(var);
        }
    }
    newEffVars.swap(effVars);
}

BDD TransitionRelation::image(const BDD &from) const {
    BDD aux = from;
    if (!swapVarsA.empty()) {
        aux = from.SwapVariables(swapVarsA, swapVarsAp);
    }
    BDD tmp = tBDD.AndAbstract(aux, existsVars);
    BDD res = tmp.SwapVariables(swapVarsS, swapVarsSp);
    if (absAfterImage) {
        //TODO: HACK: PARAMETER FIXED
        res = absAfterImage->shrinkExists(res, 10000000);
    }
    return res;
}

BDD TransitionRelation::image(const BDD &from, int maxNodes) const {
    DEBUG_MSG(cout << "Image cost " << cost << " from " << from.nodeCount() << " with " << tBDD.nodeCount();
              );
    BDD aux = from;
    if (!swapVarsA.empty()) {
        aux = from.SwapVariables(swapVarsA, swapVarsAp);
    }
    utils::Timer t;
    BDD tmp = tBDD.AndAbstract(aux, existsVars, maxNodes);
    DEBUG_MSG(cout << " tmp " << tmp.nodeCount() << " in " << t();
              );
    BDD res = tmp.SwapVariables(swapVarsS, swapVarsSp);
    DEBUG_MSG(cout << " res " << res.nodeCount() << " took " << t();
              );
    if (absAfterImage) {
        res = absAfterImage->shrinkExists(res, maxNodes);
        DEBUG_MSG(cout << " shrunk: " << res.nodeCount() << " tookTotal: " << t();
                  );
    }
    DEBUG_MSG(cout << endl;
              );

    return res;
}

BDD TransitionRelation::preimage(const BDD &from) const {
    BDD tmp = from.SwapVariables(swapVarsS, swapVarsSp);
    BDD res = tBDD.AndAbstract(tmp, existsBwVars);
    if (!swapVarsA.empty()) {
        res = res.SwapVariables(swapVarsA, swapVarsAp);
    }
    if (absAfterImage) {
        res = absAfterImage->shrinkExists(res, numeric_limits<int>::max());
    }
    return res;
}

BDD TransitionRelation::preimage(const BDD &from, int maxNodes) const {
    utils::Timer t;
    DEBUG_MSG(cout << "Image cost " << cost << " from " << from.nodeCount() << " with " << tBDD.nodeCount() << flush;
              );
    BDD tmp = from.SwapVariables(swapVarsS, swapVarsSp);
    DEBUG_MSG(cout << " tmp " << tmp.nodeCount() << " in " << t() << flush;
              );
    BDD res = tBDD.AndAbstract(tmp, existsBwVars, maxNodes);
    if (!swapVarsA.empty()) {
        res = res.SwapVariables(swapVarsA, swapVarsAp);
    }
    DEBUG_MSG(cout << "res " << res.nodeCount() << " took " << t();
              );
    if (absAfterImage) {
        res = absAfterImage->shrinkExists(res, maxNodes);
        DEBUG_MSG(cout << " shrunk: " << res.nodeCount() << " tookTotal: " << t();
                  );
    }
    DEBUG_MSG(cout << endl;
              );

    return res;
}

void TransitionRelation::merge(const TransitionRelation &t2,
                          int maxNodes) {
    assert(cost == t2.cost);
    if (cost != t2.cost) {
        cout << "Error: merging transitions with different cost: " << cost << " " << t2.cost << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
    }

    //  cout << "set_union" << endl;
    //Attempt to generate the new tBDD
    vector <int> newEffVars;
    set_union(effVars.begin(), effVars.end(),
              t2.effVars.begin(), t2.effVars.end(),
              back_inserter(newEffVars));

    BDD newTBDD = tBDD;
    BDD newTBDD2 = t2.tBDD;

    //    cout << "Eff vars" << endl;
    vector<int>::const_iterator var1 = effVars.begin();
    vector<int>::const_iterator var2 = t2.effVars.begin();
    for (vector<int>::const_iterator var = newEffVars.begin();
         var != newEffVars.end(); ++var) {
        if (var1 == effVars.end() || *var1 != *var) {
            newTBDD *= sV->biimp(*var);
        } else {
            ++var1;
        }

        if (var2 == t2.effVars.end() || *var2 != *var) {
            newTBDD2 *= sV->biimp(*var);
        } else {
            ++var2;
        }
    }
    newTBDD = newTBDD.Or(newTBDD2, maxNodes);

    if (newTBDD.nodeCount() > maxNodes) {
        DEBUG_MSG(cout << "TR size exceeded: " << newTBDD.nodeCount() <<
                  ">" << maxNodes << endl;
                  );
        throw BDDError(); //We could not sucessfully merge
    }

    tBDD = newTBDD;

    effVars.swap(newEffVars);
    existsVars *= t2.existsVars;
    existsBwVars *= t2.existsBwVars;

    for (size_t i = 0; i < t2.swapVarsS.size(); i++) {
        if (find(swapVarsS.begin(), swapVarsS.end(), t2.swapVarsS[i]) ==
            swapVarsS.end()) {
            swapVarsS.push_back(t2.swapVarsS[i]);
            swapVarsSp.push_back(t2.swapVarsSp[i]);
        }
    }

    labels.insert(t2.labels.begin(), t2.labels.end());
//    ops.insert(t2.ops.begin(), t2.ops.end());
}

    
////For each op, include relevant mutexes
//void TransitionRelation::edeletion(const std::vector<std::vector<BDD>> & notMutexBDDsByFluentFw,
//                   const std::vector<std::vector<BDD>> & notMutexBDDsByFluentBw,
//                   const std::vector<std::vector<BDD>> & exactlyOneBDDsByFluent) {
//assert(ops.size() == 1);
//assert(notMutexBDDsByFluentFw.size() == g_variable_domain.size());
//assert(notMutexBDDsByFluentBw.size() == g_variable_domain.size());
//assert(exactlyOneBDDsByFluent.size() == g_variable_domain.size());
////For each op, include relevant mutexes
//for (const auto *op : ops) {
//    for (const auto &pp : op->get_effects()) {
//        auto pre =
//            std::find_if(std::begin(op->get_preconditions()),
//                         std::end(op->get_preconditions()),
//                         [&pp] (const GlobalCondition &cond) {
//                return pp.var == cond.var;
//            }
//                         );
//
//        //edeletion bw
//        if (pre == std::end(op->get_preconditions())) {
//            //We have a post effect over this variable.
//            //That means that every previous value is possible
//            //for each value of the variable
//            for (int val = 0; val < g_variable_domain[pp.var]; val++) {
//                tBDD *= notMutexBDDsByFluentBw[pp.var][val];
//            }
//        } else {
//            //In regression, we are making true pp.pre
//            //So we must negate everything of these.
//            tBDD *= notMutexBDDsByFluentBw[pp.var] [pre->val];
//        }
//        //edeletion fw
//        tBDD *= notMutexBDDsByFluentFw[pp.var][pp.val].SwapVariables(swapVarsS, swapVarsSp);
//
//        //edeletion invariants
//        tBDD *= exactlyOneBDDsByFluent[pp.var][pp.val];
//    }
//}
//}


ostream &operator<<(std::ostream &os, const TransitionRelation &tr) {
    os << "TR(";
    for (auto l : tr.labels) {
        os << l << ", ";
    }
    return os << "): " << tr.tBDD.nodeCount() << endl;
}

}
