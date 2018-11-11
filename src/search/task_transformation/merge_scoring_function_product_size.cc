#include "merge_scoring_function_product_size.h"

#include "factored_transition_system.h"
#include "../task_representation/transition_system.h"
#include "utils.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/math.h"

using namespace std;

namespace task_transformation {
MergeScoringFunctionProductSize::MergeScoringFunctionProductSize(
    const options::Options &options)
    : max_states(options.get<int>("max_states")) {
}
vector<double> MergeScoringFunctionProductSize::compute_scores(
    const FactoredTransitionSystem &fts,
    const vector<pair<int, int>> &merge_candidates) {
    vector<double> scores;
    scores.reserve(merge_candidates.size());
    bool found_valid_merge = false;
    for (pair<int, int> merge_candidate : merge_candidates) {
        int ts_size1 = fts.get_ts(merge_candidate.first).get_size();
        int ts_size2 = fts.get_ts(merge_candidate.second).get_size();
        int score = INF;
        if (utils::is_product_within_limit(ts_size1, ts_size2, max_states)) {
            score = 0;
            found_valid_merge = true;
        }
        scores.push_back(score);
    }
    if (!found_valid_merge) {
        // Empty scores will be treated as "no valid merge found" and lead
        // to an abortion of the main loop.
        return vector<double>();
    }
    return scores;
}

string MergeScoringFunctionProductSize::name() const {
    return "product size";
}

static shared_ptr<MergeScoringFunction>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Product size limiting",
        "This scoring function assigns a merge candidate a value of 0 if the "
        "product size is at most max_size. All other candidates get a score of "
        "positive infinity.");
    parser.add_option<int>(
        "max_states",
        "A limit on the size of products.",
        "infinity");
    options::Options options = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeScoringFunctionProductSize>(options);
}

static options::PluginShared<MergeScoringFunction> _plugin("product_size", _parse);
}
