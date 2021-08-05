#ifndef SYMBOLIC_TRANSITION_RELATION_H
#define SYMBOLIC_TRANSITION_RELATION_H

#include "sym_variables.h"
#include <set>
#include <vector>

namespace symbolic {
class SymStateSpaceManager;
class OriginalStateSpace;
class SymPDB;
class SymSMAS;
class SimulationRelation;
class DominanceRelation;

/*
 * Represents a symbolic transition.
 * It has two differentiated parts: label and abstract state transitions
 * Label refers to variables not considered in the merge-and-shrink
 * Each label has one or more abstract state transitions
 */
class TransitionRelation {
    SymVariables *sV; //To call basic BDD creation methods
    int cost; // transition cost
    BDD tBDD; // bdd for making the relprod

    std::vector<int> effVars; //FD Index of eff variables. Must be sorted!!
    BDD existsVars, existsBwVars;   // Cube with variables to existentialize
    std::vector<BDD> swapVarsS, swapVarsSp; // Swap variables s to sp and viceversa
    std::vector<BDD> swapVarsA, swapVarsAp; // Swap abstraction variables

    std::set<const OperatorID *> ops; //List of operators represented by the TR
    std::set<int> labels; //List of labels represented by the TR

    const SymStateSpaceManager *absAfterImage;
public:
    //Constructor for abstraction transitions
//    TransitionRelation(SymStateSpaceManager *mgr, const DominanceRelation &sim_relations);

    //Constructor for transitions irrelevant for the abstraction
//    TransitionRelation(SymVariables *sVars, const GlobalOperator *op, int cost_);

    TransitionRelation(SymVariables *sVars, int label, const std::shared_ptr<task_representation::FTSTask> &_task);

    //Copy constructor
    TransitionRelation(const TransitionRelation &) = default;

    const std::shared_ptr<task_representation::FTSTask> &task;

    BDD image(const BDD &from) const;
    BDD preimage(const BDD &from) const;
    BDD image(const BDD &from, int maxNodes) const;
    BDD preimage(const BDD &from, int maxNodes) const;

//    void edeletion(const std::vector<std::vector<BDD>> & notMutexBDDsByFluentFw,
//		   const std::vector<std::vector<BDD>> & notMutexBDDsByFluentBw,
//		   const std::vector<std::vector<BDD>> & exactlyOneBDDsByFluent);

    void merge(const TransitionRelation &t2,
               int maxNodes);

    //shrinks the transition to another abstract state space (useful to preserve edeletion)
    void shrink(const SymStateSpaceManager &abs, int maxNodes);

    bool setMaSAbstraction(const SymStateSpaceManager &abs,
                           const BDD &bddSrc, const BDD &bddTarget);

    inline void setAbsAfterImage(const SymStateSpaceManager *abs) {
        absAfterImage = abs;
    }

    inline int getCost() const {
        return cost;
    }

    inline void set_cost(int cost_) {
        cost = cost_;
    }
    inline int nodeCount() const {
        return tBDD.nodeCount();
    }
    inline const std::set<const OperatorID *> &getOps() const {
        return ops;
    }
    inline const std::set<int> &getLabels() const {
        return labels;
    };

    inline const BDD &getBDD() const {
        return tBDD;
    }

    friend std::ostream &operator<<(std::ostream &os, const TransitionRelation &tr);

    TransitionRelation& operator=(const TransitionRelation& other) {
        if (this == &other)
            return *this;

        this->tBDD = other.tBDD;
        this->cost = other.cost;
        this->absAfterImage = other.absAfterImage;
        this->effVars = other.effVars;
        this->labels = other.labels;
        this->existsBwVars = other.existsBwVars;
        this->existsVars = other.existsVars;
        this->sV = other.sV;
        this->swapVarsA = other.swapVarsA;
        this->swapVarsAp = other.swapVarsAp;
        this->swapVarsS = other.swapVarsS;
        this->swapVarsSp = other.swapVarsSp;

        return *this;
    }
};
}
#endif
