#ifndef SYMBOLIC_ORIGINAL_STATE_SPACE_H
#define SYMBOLIC_ORIGINAL_STATE_SPACE_H

#include "sym_state_space_manager.h"

namespace symbolic {
class OriginalStateSpace : public SymStateSpaceManager {

//    void init_mutex (const std::vector<MutexGroup> &mutex_groups);
//    void init_mutex(const std::vector<MutexGroup> &mutex_groups,
//		    bool genMutexBDD, bool genMutexBDDByFluent, bool fw);

public:
    OriginalStateSpace(SymVariables *v, const SymParamsMgr &params, const std::shared_ptr<task_representation::FTSTask> &_task);
    const std::shared_ptr<task_representation::FTSTask> &task;

    //Individual TRs: Useful for shrink and plan construction
    std::map<int, std::vector <TransitionRelation>> indTRs;

    //notMutex relative for each fluent
    std::vector<std::vector<BDD>> notMutexBDDsByFluentFw, notMutexBDDsByFluentBw;
    std::vector<std::vector<BDD>> exactlyOneBDDsByFluent;

    virtual std::string tag() const override {
        return "original";
    }

    //Methods that require of mutex initialized
    inline const BDD &getNotMutexBDDFw(int var, int val) const {
        return notMutexBDDsByFluentFw[var][val];
    }

    //Methods that require of mutex initialized
    inline const BDD &getNotMutexBDDBw(int var, int val) const {
        return notMutexBDDsByFluentBw[var][val];
    }

    //Methods that require of mutex initialized
    inline const BDD &getExactlyOneBDD(int var, int val) const {
        return exactlyOneBDDsByFluent[var][val];
    }

    BDD shrinkExists(const BDD &bdd, int) const override {
        return bdd;
    }
    BDD shrinkForall(const BDD &bdd, int) const override {
        return bdd;
    }
    BDD shrinkTBDD(const BDD &bdd, int) const override  {
        return bdd;
    }

    const std::map<int, std::vector <TransitionRelation>> &getIndividualTRs() const override {
	return indTRs;
    }

    virtual ~OriginalStateSpace() = default;

};
}
#endif
