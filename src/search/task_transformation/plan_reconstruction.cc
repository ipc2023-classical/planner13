#include "plan_reconstruction.h"

#include <algorithm>
using namespace std;

namespace task_transformation {
PlanReconstructionSequence::PlanReconstructionSequence(
    vector<shared_ptr<PlanReconstruction>> plan_reconstructions)
    : plan_reconstructions(plan_reconstructions) {

    std::reverse(plan_reconstructions.begin(), plan_reconstructions.end());
}
}
