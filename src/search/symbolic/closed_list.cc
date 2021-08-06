#include "closed_list.h"

#include "sym_state_space_manager.h"
#include "sym_solution.h"
#include "sym_util.h"
#include "../symbolic_astar_NOTWORKING/sym_astar.h"

#include "../utils/debug_macros.h"
#include "../task_representation/search_task.h"
#include "../task_representation/transition_system.h"

#include <iostream>
#include <string>

#include "sym_test.h"

#include <cassert>

using namespace std;

namespace symbolic {
    ClosedList::ClosedList(const std::shared_ptr<task_representation::FTSTask> &_task) : OppositeFrontier(_task),
                                                                                         mgr(nullptr), task(_task) {}

    void ClosedList::init(SymStateSpaceManager *manager, UnidirectionalSearch *search) {
        mgr = manager;
        my_search = search;
        set<int>().swap(h_values);
        map<int, BDD>().swap(closedUpTo);
        map<int, vector<BDD>>().swap(zeroCostClosed);
        map<int, BDD>().swap(closed);
        closedTotal = mgr->zeroBDD();
        hNotClosed = 0;
        fNotClosed = 0;
    }


    void ClosedList::init(SymStateSpaceManager *manager, UnidirectionalSearch *search, const ClosedList &other) {
        mgr = manager;
        my_search = search;
        set<int>().swap(h_values);
        map<int, BDD>().swap(closedUpTo);
        map<int, vector<BDD>>().swap(zeroCostClosed);
        map<int, BDD>().swap(closed);
        closedTotal = mgr->zeroBDD();
        hNotClosed = 0;
        fNotClosed = 0;

        closedTotal = mgr->shrinkForall(other.closedTotal);
        closed[0] = closedTotal;
        newHValue(0);
    }

    void ClosedList::newHValue(int h_value) {
        h_values.insert(h_value);
    }

    void ClosedList::insert(int h, const BDD &S) {
        DEBUG_MSG(cout << "Inserting on closed " << "g=" << h << ": " << S.nodeCount() << " nodes and "
                       << mgr->getVars()->numStates(S) << " states" << endl;
        );

#ifdef DEBUG_GST
        gst_plan.checkClose(S, h, exploration);
#endif

        if (closed.count(h)) {
            assert(h_values.count(h));
            closed[h] += S;
        } else {
            closed[h] = S;
            newHValue(h);
        }

        if (mgr->hasTransitions0()) {
            zeroCostClosed[h].push_back(S);
        }
        closedTotal += S;

        //Introduce in closedUpTo
        auto c = closedUpTo.lower_bound(h);
        while (c != std::end(closedUpTo)) {
            c->second += S;
            c++;
        }
    }


    void ClosedList::setHNotClosed(int newHNotClosed) {
        if (newHNotClosed > hNotClosed) {
            hNotClosed = newHNotClosed;
            newHValue(newHNotClosed); //Add newHNotClosed to list of h values (and those of parents)
        }
    }

    void ClosedList::setFNotClosed(int f) {
        if (f > fNotClosed) {
            fNotClosed = f;
        }
    }

    void ClosedList::extract_path(const BDD &c, int h, bool fw, vector<OperatorID> &path) const {
        if (!mgr)
            return;
        DEBUG_MSG(cout << "Sym closed extract path h=" << h << " notClosed: " << hNotClosed << endl;
                          cout << "Closed: ";
                          for (auto &_c : closed)
                              cout << _c.first << " ";
                          cout << endl;
        );
        const map<int, vector<TransitionRelation>> &trs = mgr->getIndividualTRs();
        BDD cut = c;
        size_t steps0 = 0;

        if (zeroCostClosed.count(h)) {
            assert(trs.count(0));
            DEBUG_MSG(cout << "Check " << steps0 << " of " << zeroCostClosed.at(h).size() << endl;);
            while (steps0 < zeroCostClosed.at(h).size() && (cut * zeroCostClosed.at(h)[steps0]).IsZero()) {
//                DEBUG_MSG(cout << "Steps0 is not " << steps0<< " of " << zeroCostClosed.at(h).size() << endl;);
                steps0++;
            }
            DEBUG_MSG(cout << "Steps0 of h=" << h << " is " << steps0 << endl;);
            if (steps0 < zeroCostClosed.at(h).size()) {
                cut *= zeroCostClosed.at(h)[steps0];
            } else {
                DEBUG_MSG(cout << "cut not found with steps0. Try to find with preimage: " << trs.count(0) << endl;
                );
                bool foundZeroCost = false;
                for (const TransitionRelation &tr : trs.at(0)) {
                    if (foundZeroCost) {
                        break;
                    }
                    assert(tr.getLabels().size() == 1);
                    int label = *tr.getLabels().begin();
                    vector<task_representation::FTSOperator> fts_ops = task->get_search_task()->get_fts_operators_from_label(
                            label);

                    ClosedList::assertOpIdExists(tr, fts_ops);

                    BDD succ;
                    if (fw) {
                        succ = tr.preimage(cut);
                    } else {
                        succ = tr.image(cut);
                    }
                    if (succ.IsZero()) {
                        continue;
                    }

                    for (size_t newSteps0 = 0; newSteps0 < steps0; newSteps0++) {
                        BDD intersection = succ * zeroCostClosed.at(h)[newSteps0];
                        if (!intersection.IsZero()) {
                            steps0 = newSteps0;
                            foundZeroCost = true;
                            break;
                        }
                    }
                    if (!foundZeroCost) {
                        continue;
                    }

                    bool foundOpId = false;

                    for (const auto &op : fts_ops) {
                        succ = setSuccWithEffects(cut, op, fw, tr);
                        if (succ.IsZero()) {
                            continue;
                        }
                        BDD intersection = succ * zeroCostClosed.at(h)[steps0];
                        if (!intersection.IsZero()) {
                            path.push_back(op.get_id());
                            cut = intersection;
                            foundOpId = true;
                            break;
                        }
                    }
                    if (foundOpId)
                        break;
                }
                if (!foundZeroCost) {
                    DEBUG_MSG(cout << "cut not found with steps0. steps0=0:" << endl;
                    );

                    steps0 = 0;
                }
            }
        }

        while (h > 0 || steps0 > 0) {
            cout << "h > 0 || steps0 > 0" << endl;
            DEBUG_MSG(cout << "h=" << h << " and steps0=" << steps0 << endl;);
            if (steps0 > 0) {
                cout << "steps0 > 0" << endl;
                bool foundZeroCost = false;
                //Apply 0-cost operators
                if (trs.count(0)) {
                    for (const TransitionRelation &tr : trs.at(0)) {
                        assert(tr.getLabels().size() == 1);
                        int label = *tr.getLabels().begin();
                        vector<task_representation::FTSOperator> fts_ops = task->get_search_task()
                                ->get_fts_operators_from_label(label);
                        ClosedList::assertOpIdExists(tr, fts_ops);

                        BDD succ;
                        if (fw) {
                            succ = tr.preimage(cut);
                        } else {
                            succ = tr.image(cut);
                        }
                        if (succ.IsZero()) {
                            continue;
                        }

                        BDD intersection;
                        for (size_t newSteps0 = 0; newSteps0 < steps0; newSteps0++) {
                            intersection = succ * zeroCostClosed.at(h)[newSteps0];
                            if (!intersection.IsZero()) {
                                steps0 = newSteps0;
                                foundZeroCost = true;
                                break;
                            }
                        }
                        if (!foundZeroCost) {
                            continue;
                        }

                        // bool foundOpId = false;
                        for (const auto &op : fts_ops) {
                            succ = setSuccWithEffects(cut, op, fw, tr);

                            intersection = succ * zeroCostClosed.at(h)[steps0];
                            if (!intersection.IsZero()) {
                                path.push_back(op.get_id());
                                DEBUG_MSG(cout << "Adding " << op.get_id().get_index() << ", cost: "
                                               << op.get_cost() << endl;);
                                cut = intersection;
                                // foundOpId = true;
                                break;
                            }
                        }
                        assert(foundZeroCost);
                        // assert(foundOpId);
                        break;
                    }
                }
                DEBUG_MSG(cout << "FoundZeroCost: " << foundZeroCost << endl;);
                if (!foundZeroCost) {
//                    DEBUG_MSG(
//                            cout << "Force steps0 = 0" << endl;
//                            for (int newSteps0 = 0; newSteps0 <= int(steps0); newSteps0++) {
//                                cout << "Steps0: " << newSteps0 << ": ";
//                                zeroCostClosed.at(h)[newSteps0].print(0, 2);
//                                cout << "CUT: ";
//                                cut.print(0, 1);
//                            }
//                    );
                    assert(h > 0);

                    steps0 = 0;
                }
            }

            if (h > 0 && steps0 == 0) {
                DEBUG_MSG(cout << "h=" << h << " and steps0=" << steps0 << endl;);
                bool found = false;
                for (auto key : trs) { //TODO: maybe is best to use the inverse order
                    if (found)
                        break;
                    int newH = h - key.first;
                    if (key.first == 0 || closed.count(newH) == 0)
                        continue;
                    for (TransitionRelation &tr : key.second) {
                        /*DEBUG_MSG(cout << "Check. size: " << tr.getLabels().size() << " label: "
                                       << (*(tr.getLabels().begin())) << " of cost " << key.first << " in h=" << newH
                                       << endl;);*/

                        assert (tr.getLabels().size() == 1);
                        int label = *tr.getLabels().begin();
                        vector<task_representation::FTSOperator> fts_ops = task->get_search_task()
                                ->get_fts_operators_from_label(label);
                        for (const auto &op : fts_ops) {
                            BDD succ;
                            succ = setSuccWithEffects(cut, op, fw, tr);
                            if (succ.IsZero()) {
                                continue;
                            }
                            BDD intersection = succ * closed.at(newH);
                            /*DEBUG_MSG(cout << "Image computed: "; succ.print(0,1);
                              cout << "closed at newh: "; closed.at(newH).print(0,1);
                              cout << "Intersection: "; intersection.print(0,1););*/
                            if (!intersection.IsZero()) {
                                h = newH;
                                cut = succ;
                                steps0 = 0;
                                if (zeroCostClosed.count(h)) {
                                    while ((cut * zeroCostClosed.at(h)[steps0]).IsZero()) {
//                                        DEBUG_MSG(cout << "r Steps0 is not " << steps0<< " of " << zeroCostClosed.at(h).size() << endl;);
                                        steps0++;
                                        assert(steps0 < zeroCostClosed.at(newH).size());
                                    }

//                                    DEBUG_MSG(cout << "r Steps0 of h=" << h << " is " << steps0 << endl;);

                                    cut *= zeroCostClosed.at(h)[steps0];
                                }
                                path.push_back(op.get_id());
                                DEBUG_MSG(
                                        cout << "Adding " << op.get_id().get_index() << ", cost: " << op.get_cost()
                                             << endl;);

                                found = true;
                                break;
                            }

                        }
                        if (found)
                            break;
                    }
                }
                if (!found) {
                    cerr << "Error: ClosedList.cc fail. h: " << h << endl;
                    cerr << "Error: ClosedList.cc could not find path" << endl;
                    utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
                }
            }
        }

        DEBUG_MSG(cout << "Sym closed extracted path" << endl;
        );
    }

    void ClosedList::assertOpIdExists(const TransitionRelation& tr, const vector<task_representation::FTSOperator>& fts_ops) const {
        (void) tr;
        BDD all_effects = mgr->zeroBDD();
        for (const auto& op : fts_ops) {
            BDD op_eff_bdd = mgr->oneBDD();
            for (const auto& eff : op.get_effects()) {
                op_eff_bdd *= mgr->getVars()->targetStateBDD(eff.var, eff.value);
                assert(!op_eff_bdd.IsZero());
            }
            all_effects += op_eff_bdd;
        }

        assert(tr.getBDD() * all_effects == tr.getBDD());
    }

    BDD ClosedList::setSuccWithEffects(const BDD &cut,
                                       const task_representation::FTSOperator &op, int fw,
                                       const TransitionRelation &tr) const {
        BDD succ;
        // create operatorID bdd
        BDD effect_bdd = mgr->oneBDD();// conjunction of all effects
        for (auto eff : op.get_effects()) {
            effect_bdd *= mgr->getVars()->sourceStateBDD(eff.var, eff.value);
        }

        if (fw) {
            BDD cut_with_effect_bdd = cut * effect_bdd;
            if (cut_with_effect_bdd.IsZero())
                return mgr->zeroBDD();

            succ = tr.preimage(cut_with_effect_bdd);
        } else {
            succ = tr.image(cut) * effect_bdd;
        }

        return succ;
    }

    SymSolution ClosedList::checkCut(UnidirectionalSearch *search, const BDD &states, int g, bool fw) const {
        BDD cut_candidate = states * closedTotal;
        if (cut_candidate.IsZero()) {
            return SymSolution(task); //No solution yet :(
        }

        for (const auto &closedH : closed) {
            int h = closedH.first;

            DEBUG_MSG(cout << "Check cut of g=" << g << " with h=" << h << endl;);
            BDD cut = closedH.second * cut_candidate;
            if (!cut.IsZero()) {
                if (fw) //Solution reconstruction will fail
                    return SymSolution(search, my_search, g, h, cut, task);
                else
                    return SymSolution(my_search, search, h, g, cut, task);
            }
        }

        cerr << "Error: Cut with closedTotal but not found on closed" << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
    }

    void ClosedList::getHeuristic(vector<ADD> &heuristics,
                                  vector<int> &maxHeuristicValues) const {
        int previousMaxH = 0; //Get the previous value of max h
        if (!maxHeuristicValues.empty()) {
            previousMaxH = maxHeuristicValues.back();
        }
        /*If we did not complete one step, and we do not surpass the previous maxH
          we do not have heuristic*/
        if (closed.size() <= 1 && hNotClosed <= previousMaxH) {
            cout << "Heuristic not inserted: "
                 << hNotClosed << " " << closed.size() << endl;
            return;
        }

        ADD h = getHeuristic(previousMaxH);

        //  closed.clear(); //The heuristic values have been stored in the ADD.
        cout << "Heuristic with maxValue: " << hNotClosed
             << " ADD size: " << h.nodeCount() << endl;

        maxHeuristicValues.push_back(hNotClosed);
        heuristics.push_back(h);
    }


    ADD ClosedList::getHeuristic(int previousMaxH /*= -1*/) const {
        /* When zero cost operators have been expanded, all the states in non reached
           have a h-value strictly greater than frontierCost.
           They can be frontierCost + min_action_cost or the least bucket in open. */
        /*  int valueNonReached = frontierCost;
            if(frontierCost >= 0 && zeroCostExpanded){
            cout << "Frontier cost is " << frontierCost << endl;
            closed[frontierCost] = S;
            valueNonReached = frontierCost + mgr->getMinTransitionCost();
            if(!open.empty()){
            valueNonReached = min(open.begin()->first,
            valueNonReached);
            }
            }*/
        BDD statesWithHNotClosed = !closedTotal;
        ADD h = mgr->mgr()->constant(-1);
        //cout << "New heuristic with h [";
        for (auto &it : closed) {
            //cout << it.first << " ";
            int h_val = it.first;

            /*If h_val < previousMaxH we can put it to that value
              However, we only do so if it is less than hNotClosed
              (or we will think that we have not fully determined the value)*/
            if (h_val < previousMaxH && previousMaxH < hNotClosed) {
                h_val = previousMaxH;
            }
            if (h_val != hNotClosed) {
                h += it.second.Add() * mgr->mgr()->constant(h_val + 1);
            } else {
                statesWithHNotClosed += it.second;
            }
        }
        cout << hNotClosed << "]" << endl;

        if (hNotClosed != numeric_limits<int>::max() && hNotClosed >= 0 && !statesWithHNotClosed.IsZero()) {
            h += statesWithHNotClosed.Add() * mgr->mgr()->constant(hNotClosed + 1);
        }

        return h;
    }


    void ClosedList::statistics() const {
        // cout << "h (eval " << num_calls_eval << ", not_closed" << time_eval_states << "s, closed " << time_closed_states
        //   << "s, pruned " << time_pruned_states << "s, some " << time_prune_some
        //   << "s, all " << time_prune_all  << ", children " << time_prune_some_children << "s)";
    }

    const std::set<int> &ClosedList::getHValues() {
        assert(h_values.count(hNotClosed));

        return h_values;
    }


    double ClosedList::average_hvalue() const {
        double averageHeuristic = 0;
        double heuristicSize = 0;
        for (const auto &item : closed) {
            double currentSize = mgr->getVars()->numStates(item.second);
            DEBUG_MSG(cout << item.first << " " << currentSize << endl;);
            averageHeuristic += currentSize * item.first;
            heuristicSize += currentSize;
        }
        double notClosedSize = mgr->getVars()->numStates(notClosed());
        heuristicSize += notClosedSize;
        int maxH = (closed.empty() ? 0 : closed.rbegin()->first);

        DEBUG_MSG(cout << maxH << " " << notClosedSize << endl;
                          cout << "Max size: " << heuristicSize << endl << endl;);

        averageHeuristic += notClosedSize * maxH;
        return averageHeuristic / heuristicSize;
    }
}
