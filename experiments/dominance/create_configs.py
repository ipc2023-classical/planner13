#! /usr/bin/env python

import os
import sys
import subprocess

def config_name(name1, name2):
    return f"{name1}-{name2}" if len(name1) else name2

REVS_CONFIGS = {
    ("61a0d7fa82f4ced564baab92fc050f5f00a5b8c4", "") : [
        (config_name(t1, s1), t2+s2) for (t1, t2) in [('',[]),
                                                      ('atomiclr', ['--transform', 'transform_merge_and_shrink(label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=false,run_main_loop=false,max_time=900,cost_type=one)'])] for (s1, s2) in [('blind', ["--search", 'astar(blind)'])]
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
