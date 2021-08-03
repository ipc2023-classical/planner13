#! /usr/bin/env python

import os
import sys
import subprocess

def config_name(name1, name2):
    return f"{name1}-{name2}" if len(name1) else name2


TRANSFORMATION_CONFIGS = {
    '': [],
    'atomic-lr' :  ['--transform', 'transform_merge_and_shrink(label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=false,run_main_loop=false,max_time=900)'],
    'atomic-bisim' : ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=false)"],
    'dfp1000-bisim' : ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)"],
    'miasm1000-bisim' : ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=1000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)"],
    'atomic-weakbisim' : ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_weak_bisimulation(ignore_irrelevant_tau_groups=false),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=false)"],
    'dfp1000-weakbisim' : ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_weak_bisimulation(ignore_irrelevant_tau_groups=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)"],
    'miasm1000-weakbisim' : ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_weak_bisimulation(ignore_irrelevant_tau_groups=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=1000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)"]
}


SEARCH_CONFIGS = {
    'sbd' : ["--search", "sbd()"],
    'sfw' : ["--search", 'sfw()'],
    'sbw' : ["--search", 'sbw()']
}

REVS_CONFIGS = {
    ("0a98e74b3a3e90bbd3a733ecd8692a76281fb19c", "21-08-03") : [
        (config_name(t1, s1), t2+s2) for (t1, t2) in TRANSFORMATION_CONFIGS.items() for (s1, s2) in SEARCH_CONFIGS.items()
    ]
}

template = """#! /usr/bin/env python

import os
import project

REPO = project.get_repo_base()
BENCHMARKS_DIR = '{{}}/../benchmarks'.format(REPO)

if project.REMOTE:
    SUITE = project.DEFAULT_OPTIMAL_SUITE
    ENV = project.BaselSlurmEnvironment(partition="naples",email="bx56lg@cs.aau.dk")
else:
    SUITE = project.DEFAULT_TEST_SUITE
    ENV = project.LocalEnvironment(processes=2)

BUILD_OPTIONS = []
DRIVER_OPTIONS = ["--overall-time-limit", "1800"]

exp = project.CommonExperiment(environment=ENV)

exp.add_algorithm(
            "{algo_name}",
            REPO,
            "{rev}",
            {config},
            build_options=BUILD_OPTIONS,
            driver_options=DRIVER_OPTIONS,
        )
exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.run_steps()
"""



if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print ("please specify prefix-path")
        exit()

    prefix = sys.argv[1]


    for rev, rev_nick in REVS_CONFIGS:
        for config_nick, config in REVS_CONFIGS[(rev, rev_nick)]:
            algo_name = f"{rev_nick}:{config_nick}" if len(rev_nick) else f"{config_nick}"
            content = template.format(**locals())

            name = prefix + config_nick + ".py"

            if os.path.exists(name):
                print ("Skipping: ", name)
                continue
            with open(name, "w") as file:
                file.write(content)
                subprocess.call(["chmod", "+x", name])
