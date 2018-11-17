#include "shrink_composite.h"

#include "../task_representation/transition_system.h"
#include "factored_transition_system.h"

#include "shrink_bisimulation.h"
#include "shrink_own_labels.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace task_transformation {

ShrinkComposite::ShrinkComposite(const std::vector<std::shared_ptr<ShrinkStrategy> > & strategies_)
    : ShrinkStrategy(), 
      strategies(strategies_) {    
}

ShrinkComposite::ShrinkComposite(const Options &opts)
    : ShrinkStrategy(), 
      strategies(opts.get_list<shared_ptr<ShrinkStrategy>>("strategies")) {    
}

string ShrinkComposite::name() const {
    return "composite";
}

     StateEquivalenceRelation ShrinkComposite::compute_equivalence_relation(
        const FactoredTransitionSystem &fts,
        int index,
        int target_size) const  {
         StateEquivalenceRelation  best_option;
         size_t best_size = std::numeric_limits<size_t>::max();
         for (auto & st : strategies) {
             assert(index >= 0);
             assert(fts.is_active(index));
             auto option = st->compute_equivalence_relation(fts, index, target_size);
             assert(fts.is_active(index));
             if (option.size() < best_size) {
                 best_size = option.size();
                 best_option=move(option);
             }
         }

         return best_option;

        
    }


     bool ShrinkComposite::apply_shrinking_transformation(FactoredTransitionSystem &fts,
                                                Verbosity verbosity) const  {
        bool changes = false;
        for (auto & st : strategies) {
            changes |= st->apply_shrinking_transformation(fts, verbosity);
        }
        return changes;
    }
    
     bool ShrinkComposite::apply_shrinking_transformation(FactoredTransitionSystem &fts,
                                                Verbosity verbosity, int & index) const  {
        bool changes = false;
        for (auto & st : strategies) {
            assert(index >= 0);
            changes |= st->apply_shrinking_transformation(fts, verbosity, index);
            if (index== -1) {
                assert(changes);
                return changes;
            }
        }
        return changes;
    }

    bool ShrinkComposite::requires_init_distances() const {
        for (auto & st : strategies) {
            if (st->requires_init_distances()) {
                return true;
            }
        }
        return false;
    }
    
    bool ShrinkComposite::requires_goal_distances() const {
        for (auto & st : strategies) {
            if (st->requires_goal_distances()) {
                return true;
            }
        }
        return false;
    }

static shared_ptr<ShrinkStrategy> _parse(OptionParser &parser) {
    parser.add_list_option<shared_ptr<ShrinkStrategy>>("strategies", 
						       "list of strategoes");

    Options opts = parser.parse();

    if (parser.help_mode())
        return 0;

    opts.verify_list_non_empty<shared_ptr<ShrinkStrategy>>("strategies");

    if (!parser.dry_run())
        return make_shared<ShrinkComposite>(opts);
    else
        return nullptr;
}

    static shared_ptr<ShrinkStrategy> _parse_perfect(OptionParser &/*parser*/) {
        Options opts; 

        vector<shared_ptr<ShrinkStrategy>> strategies;
        strategies.push_back(ShrinkOwnLabels::create_default());
        strategies.push_back(ShrinkBisimulation::create_default_perfect());


        return make_shared<ShrinkComposite> (strategies);
    }

    void ShrinkComposite::dump_strategy_specific_options() const {
        for(auto st: strategies) {
            st->dump_options();
        }
    }

static PluginShared<ShrinkStrategy> _plugin("shrink_composite", _parse);

//Creates a default composite of shrink own labels and shrink bisimulation
static PluginShared<ShrinkStrategy> _plugin_perfect("shrink_own_bisimulation", _parse_perfect); 

}
