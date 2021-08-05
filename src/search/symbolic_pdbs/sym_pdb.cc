#include "sym_pdb.h"

#include "../symbolic/sym_util.h"
#include "../symbolic/transition_relation.h"
#include "../symbolic/original_state_space.h"

#include "../utils/system.h"
#include "../utils/debug_macros.h"
#include "../operator_cost.h"
#include "../symbolic/sym_variables.h"
#include "../task_representation/transition_system.h"


using namespace std;

namespace symbolic {
    SymPDB::SymPDB(const OriginalStateSpace &parent,
                   const std::set<int> &relevantVars, const std::shared_ptr<task_representation::FTSTask> &_task) :
            SymStateSpaceManager(parent.getVars(), parent.getParams(), _task, relevantVars), task(_task) {
        std::set<int> nonRelVars;
        for (size_t i = 0; i < size_t(task->get_size()); ++i) {
            if (!isRelevantVar(i)) {
                nonRelVars.insert(i);
            }
        }
//        assert(nonRelVarsCube.IsCube()); // todo this assertion will always error
        assert(isAbstracted());

        nonRelVarsCube = vars->getCubePre(nonRelVars);    // * vars->getCubep(nonRelVars);
        nonRelVarsCubeWithPrimes = nonRelVarsCube * vars->getCubeEff(nonRelVars);

        // Init initial state
        vector<pair<int, int>> abstract_ini;
        for (int var : relevant_vars) {
            abstract_ini.push_back(
                    std::pair<int, int>(var, task->get_ts(var).get_init_state())); // Maybe use emplace_back
        }

        initialState = vars->getInitialStateBDD();
        goal = vars->getGoalBDD(relevantVars);

        //Init mutex

        //Init dead ends: Both are put into notDeadEndFw for the case
        //of abstract searches
        for (const auto &bdd : parent.getNotDeadEnds(false)) {
            notDeadEndFw.push_back(bdd);
        }

        for (const auto &bdd : parent.getNotDeadEnds(true)) {
            notDeadEndFw.push_back(bdd);
        }

        mergeBucketAnd(notDeadEndFw);

        for (auto &bdd : notDeadEndFw) {
            bdd = shrinkExists(bdd, p.max_mutex_size);
        }

        //Init transitions
        std::map<int, std::vector<TransitionRelation>> indTRs;
        std::map<int, std::vector<TransitionRelation>> failedToShrink;
        for (const auto &indTRsCost : parent.getIndividualTRs()) {
            for (const auto &trParent : indTRsCost.second) {
                TransitionRelation absTransition = TransitionRelation(trParent);
                assert (absTransition.getLabels().size() == 1);
                int label = *(absTransition.getLabels().begin());
                assert(task->get_label_cost(label) == indTRsCost.first);
//                if(!is_relevant_op(**(absTransition.getOps().begin()))) continue;

                int cost = task->get_label_cost(label);
                if (cost != absTransition.getCost())
                    absTransition.set_cost(cost);
                try {
                    vars->setTimeLimit(p.max_aux_time);
                    absTransition.shrink(*this, p.max_aux_nodes);
                    vars->unsetTimeLimit();
                    indTRs[cost].push_back(absTransition);
                } catch (BDDError e) {
                    vars->unsetTimeLimit();
                    failedToShrink[cost].push_back(absTransition);
                }
            }
        }

        init_transitions(indTRs);

        for (auto &trs : transitions) {
            merge(vars, trs.second, mergeTR, p.max_aux_time, p.max_tr_size);
        }

        //Use Shrink after img in all the transitions that failedToShrink
        DEBUG_MSG(cout << "Failed to shrink: " << (failedToShrink.empty() ? "no" : "yes") << endl;);

        for (auto &failedTRs : failedToShrink) {
            merge(vars, failedTRs.second, mergeTR, p.max_aux_time, p.max_tr_size);
            for (auto &tr : failedTRs.second) {
                tr.setAbsAfterImage(this);
                transitions[failedTRs.first].push_back(tr);
            }
        }

        DEBUG_MSG(cout << "Finished init trs: " << transitions.size() << endl;);
        assert(!hasTR0 || transitions.count(0));
    }

    BDD SymPDB::shrinkExists(const BDD &bdd, int maxNodes) const {
        return bdd.ExistAbstract(nonRelVarsCube, maxNodes);
    }

    BDD SymPDB::shrinkTBDD(const BDD &bdd, int maxNodes) const {
        return bdd.ExistAbstract(nonRelVarsCubeWithPrimes, maxNodes);
    }

    BDD SymPDB::shrinkForall(const BDD &bdd, int maxNodes) const {
        return bdd.UnivAbstract(nonRelVarsCube, maxNodes);
    }

    std::string SymPDB::tag() const {
        return "PDB";
    }

    void SymPDB::print(std::ostream &os, bool fullInfo) const {
        os << "PDB (" << relevant_vars.size() << "/" << (task->get_size()) << "): ";
        for (int v : relevant_vars) {
            os << v << " ";
        }
        if (fullInfo && isAbstracted()) {
            os << " [";
            for (int v : relevant_vars)
                os << v << " ";
            os << "]";
            os << endl << "Considered propositions: ";
            for (int v : relevant_vars) {
                os << v << ": ";
//		for (auto &prop : g_fact_names[v]) //TODO ask
//		    os << prop << ", ";
                os << endl;
            }
            os << endl;
        }
    }

}
