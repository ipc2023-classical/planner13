#include "plan_reconstruction.h"

#include <algorithm>
using namespace std;

namespace task_transformation {
PlanReconstructionSequence::PlanReconstructionSequence(
    vector<shared_ptr<PlanReconstruction>> plan_reconstructions_)
    : plan_reconstructions(plan_reconstructions_) {

    std::reverse(plan_reconstructions.begin(), plan_reconstructions.end());
}

    std::ostream& operator<<(std::ostream& o, const PlanReconstruction& b) {
        b.print(o);
        return o;
    }

}
