#include "plan_reconstruction.h"

using namespace std;

namespace task_transformation {
PlanReconstructionSequence::PlanReconstructionSequence(
    vector<unique_ptr<PlanReconstruction>> &&plan_reconstructions)
    : plan_reconstructions(move(plan_reconstructions)) {
}

PlanReconstructionSequence::~PlanReconstructionSequence() {
    for (auto &plan_reconstruction : plan_reconstructions) {
        plan_reconstruction = nullptr;
    }
}
}
