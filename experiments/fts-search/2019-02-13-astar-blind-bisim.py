#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os
import subprocess

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run, get_experiment_name

REVISION = 'b4e37e318a82'

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/repos/downward/benchmarks')
    suite = DEFAULT_OPTIMAL_SUITE
    environment = BaselSlurmEnvironment(email="silvan.sievers@unibas.ch", export=["PATH"])

    if is_test_run():
        suite = ['gripper:prob01.pddl', 'depot:p01.pddl', 'mystery:prob07.pddl']
        environment = LocalEnvironment(processes=4)

    configs = {
        IssueConfig('astar-blind-atomic', ["--search", "astar(blind)"]),
        # IssueConfig('astar-blind-transform-atomic-labelreduction', ["--transform", "transform_merge_and_shrink(label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=false,run_main_loop=false)", "--search", "astar(blind)"]),

        IssueConfig('astar-blind-transform-atomic-bisim-labelreduction', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=false)", "--search", "astar(blind)"]),
        # IssueConfig('astar-blind-transform-full-bisim-labelreduction-dfp100-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(100),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),
        # IssueConfig('astar-blind-transform-full-bisim-labelreduction-dfp1000-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),
        # IssueConfig('astar-blind-transform-full-bisim-labelreduction-dfp10000-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(10000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),
        # IssueConfig('astar-blind-transform-full-bisim-labelreduction-miasm100-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(100),sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=100,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),
        # IssueConfig('astar-blind-transform-full-bisim-labelreduction-miasm1000-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=1000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),
        # IssueConfig('astar-blind-transform-full-bisim-labelreduction-miasm10000-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(10000),sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=10000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(max_time=300,atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),
    }

    exp = IssueExperiment(
        revisions=revisions,
        configs=configs,
        environment=environment,
    )
    exp.add_suite(benchmarks_dir, suite)

    exp.add_parser(exp.EXITCODE_PARSER)
    exp.add_parser(exp.TRANSLATOR_PARSER)
    exp.add_parser(exp.SINGLE_SEARCH_PARSER)
    exp.add_parser(exp.PLANNER_PARSER)
    exp.add_parser('fts-parser.py')

    ms_algorithm_time = Attribute('ms_algorithm_time', absolute=False, min_wins=True, functions=[geometric_mean])
    ms_atomic_algorithm_time = Attribute('ms_atomic_algorithm_time', absolute=False, min_wins=True, functions=[geometric_mean])
    ms_memory_delta = Attribute('ms_memory_delta', absolute=False, min_wins=True)
    fts_transformation_time = Attribute('fts_transformation_time', absolute=False, min_wins=True, functions=[geometric_mean])
    fts_search_task_construction_time = Attribute('fts_search_task_construction_time', absolute=False, min_wins=True, functions=[geometric_mean])
    fts_plan_reconstruction_time = Attribute('fts_plan_reconstruction_time', absolute=False, min_wins=True, functions=[geometric_mean])
    atomic_task_constructed = Attribute('atomic_task_constructed', absolute=True, min_wins=False)
    extra_attributes = [
        ms_algorithm_time,
        ms_atomic_algorithm_time,
        ms_memory_delta,
        fts_transformation_time,
        fts_search_task_construction_time,
        fts_plan_reconstruction_time,
        atomic_task_constructed,
    ]

    attributes = list(exp.DEFAULT_TABLE_ATTRIBUTES)
    attributes.extend(extra_attributes)

    exp.add_step('build', exp.build)
    exp.add_step('start', exp.start_runs)
    exp.add_fetcher(name='fetch')

    exp.add_absolute_report_step(attributes=attributes, filter_algorithm=[
        '{}-astar-blind-atomic'.format(REVISION),
        '{}-astar-blind-transform-atomic-labelreduction'.format(REVISION),
        '{}-astar-blind-transform-atomic-bisim-labelreduction'.format(REVISION),
        '{}-astar-blind-transform-full-bisim-labelreduction-dfp100-t900'.format(REVISION),
        '{}-astar-blind-transform-full-bisim-labelreduction-dfp1000-t900'.format(REVISION),
        '{}-astar-blind-transform-full-bisim-labelreduction-dfp10000-t900'.format(REVISION),
        '{}-astar-blind-transform-full-bisim-labelreduction-miasm100-t900'.format(REVISION),
        '{}-astar-blind-transform-full-bisim-labelreduction-miasm1000-t900'.format(REVISION),
        '{}-astar-blind-transform-full-bisim-labelreduction-miasm10000-t900'.format(REVISION),
    ])

    BASELINE_REV = 'fts-search-base'
    exp.add_fetcher('data/2018-11-05-regular-baselines-eval', filter_algorithm=[
        '{}-astar-blind'.format(BASELINE_REV),
    ],merge=True)

    outfile = os.path.join(exp.eval_dir, get_experiment_name() + '-regular-vs-fts.html')
    exp.add_report(
        ComparativeReport(
            algorithm_pairs=[
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-atomic'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-atomic-labelreduction'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-atomic-bisim-labelreduction'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-dfp100-t900'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-dfp1000-t900'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-dfp10000-t900'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-miasm100-t900'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-miasm1000-t900'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-miasm10000-t900'.format(REVISION)),
            ],
            format='html',
            attributes=attributes,
        ),
        outfile=outfile,
    )
    exp.add_step('publish-{}'.format(outfile), subprocess.call, ['publish', outfile])

    OLD_REV = '01551daf787f'
    exp.add_fetcher('data/2018-11-19-astar-blind-bisim-eval', filter_algorithm=[
        '{}-astar-blind-atomic'.format(OLD_REV),
        '{}-astar-blind-transform-atomic-bisim-labelreduction'.format(OLD_REV),
    ],merge=True)

    outfile = os.path.join(exp.eval_dir, get_experiment_name() + '-old-vs-new.html')
    exp.add_report(
        ComparativeReport(
            algorithm_pairs=[
                ('{}-astar-blind-atomic'.format(OLD_REV), '{}-astar-blind-atomic'.format(REVISION)),
                ('{}-astar-blind-transform-atomic-bisim-labelreduction'.format(OLD_REV), '{}-astar-blind-transform-atomic-bisim-labelreduction'.format(REVISION)),
            ],
            format='html',
            attributes=attributes,
        ),
        outfile=outfile,
    )
    exp.add_step('publish-{}'.format(outfile), subprocess.call, ['publish', outfile])

    exp.run_steps()

main(revisions=[REVISION])