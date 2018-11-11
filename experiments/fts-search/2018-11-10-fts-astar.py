#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run, get_experiment_name

REVISION = 'ec29e8bc29ed'

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/repos/downward/benchmarks')
    suite = DEFAULT_OPTIMAL_SUITE
    environment = BaselSlurmEnvironment(email="silvan.sievers@unibas.ch", export=["PATH"])

    if is_test_run():
        suite = ['gripper:prob01.pddl', 'depot:p01.pddl', 'mystery:prob07.pddl']
        environment = LocalEnvironment(processes=4)

    configs = {
        IssueConfig('astar-blind-atomic', ["--search", "astar(blind)"]),
        IssueConfig('astar-blind-transform-atomic-labelreduction', ["--transform", "transform_merge_and_shrink(label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),run_main_loop=false)", "--search", "astar(blind)"]),
        IssueConfig('astar-blind-transform-atomic-bisim', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),shrink_atomic_fts=true,run_main_loop=false)", "--search", "astar(blind)"]),
        IssueConfig('astar-blind-transform-atomic-bisim-labelreduction', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=false)", "--search", "astar(blind)"]),
        IssueConfig('astar-blind-transform-full-bisim-labelreduction-dfp50k-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(50000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),

        IssueConfig('astar-hmax-atomic', ["--search", "astar(hmax)"]),
        IssueConfig('astar-hmax-transform-atomic-labelreduction', ["--transform", "transform_merge_and_shrink(label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),run_main_loop=false)", "--search", "astar(hmax)"]),
        IssueConfig('astar-hmax-transform-atomic-bisim', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),shrink_atomic_fts=true,run_main_loop=false)", "--search", "astar(hmax)"]),
        IssueConfig('astar-hmax-transform-atomic-bisim-labelreduction', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=false)", "--search", "astar(hmax)"]),
        IssueConfig('astar-hmax-transform-full-bisim-labelreduction-dfp50k-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(50000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=false,max_time=900)", "--search", "astar(hmax)"]),
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

    attributes = exp.DEFAULT_TABLE_ATTRIBUTES

    exp.add_step('build', exp.build)
    exp.add_step('start', exp.start_runs)
    exp.add_fetcher(name='fetch')

    exp.add_absolute_report_step(attributes=attributes, filter_algorithm=[
        '{}-astar-blind-atomic'.format(REVISION),
        '{}-astar-blind-transform-atomic-labelreduction'.format(REVISION),
        '{}-astar-blind-transform-atomic-bisim'.format(REVISION),
        '{}-astar-blind-transform-atomic-bisim-labelreduction'.format(REVISION),
        '{}-astar-blind-transform-full-bisim-labelreduction-dfp590k-t900'.format(REVISION),
        '{}-astar-hmax-atomic'.format(REVISION),
        '{}-astar-hmax-transform-atomic-labelreduction'.format(REVISION),
        '{}-astar-hmax-transform-atomic-bisim'.format(REVISION),
        '{}-astar-hmax-transform-atomic-bisim-labelreduction'.format(REVISION),
        '{}-astar-hmax-transform-full-bisim-labelreduction-dfp590k-t900'.format(REVISION),
    ])

    BASELINE_REV = 'fts-search-base'
    exp.add_fetcher('data/2018-11-05-regular-baselines-eval', filter_algorithm=[
        '{}-astar-blind'.format(BASELINE_REV),
        '{}-astar-hmax'.format(BASELINE_REV),
    ],merge=True)

    exp.add_report(
        ComparativeReport(
            algorithm_pairs=[
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-atomic'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-atomic-labelreduction'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-atomic-bisim'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-atomic-bisim-labelreduction'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-dfp590k-t900'.format(REVISION)),
                ('{}-astar-hmax'.format(BASELINE_REV), '{}-astar-hmax-atomic'.format(REVISION)),
                ('{}-astar-hmax'.format(BASELINE_REV), '{}-astar-hmax-transform-atomic-labelreduction'.format(REVISION)),
                ('{}-astar-hmax'.format(BASELINE_REV), '{}-astar-hmax-transform-atomic-bisim'.format(REVISION)),
                ('{}-astar-hmax'.format(BASELINE_REV), '{}-astar-hmax-transform-atomic-bisim-labelreduction'.format(REVISION)),
                ('{}-astar-hmax'.format(BASELINE_REV), '{}-astar-hmax-transform-full-bisim-labelreduction-dfp590k-t900'.format(REVISION)),
            ],
            format='html',
            attributes=attributes,
        ),
        outfile = os.path.join(exp.eval_dir, get_experiment_name() + '-regular-vs-fts.html'),
    )

    exp.add_report(
        ComparativeReport(
            algorithm_pairs=[
                ('{}-astar-blind-atomic'.format(REVISION), '{}-astar-blind-transform-atomic-labelreduction'.format(REVISION)),
                ('{}-astar-blind-atomic'.format(REVISION), '{}-astar-blind-transform-atomic-bisim'.format(REVISION)),
                ('{}-astar-blind-atomic'.format(REVISION), '{}-astar-blind-transform-atomic-bisim-labelreduction'.format(REVISION)),
                ('{}-astar-hmax-atomic'.format(REVISION), '{}-astar-hmax-transform-atomic-labelreduction'.format(REVISION)),
                ('{}-astar-hmax-atomic'.format(REVISION), '{}-astar-hmax-transform-atomic-bisim'.format(REVISION)),
                ('{}-astar-hmax-atomic'.format(REVISION), '{}-astar-hmax-transform-atomic-bisim-labelreduction'.format(REVISION)),
            ],
            format='html',
            attributes=attributes,
        ),
        outfile = os.path.join(exp.eval_dir, get_experiment_name() + '-atomic-vs-transform.html'),
    )

    exp.run_steps()

main(revisions=[REVISION])