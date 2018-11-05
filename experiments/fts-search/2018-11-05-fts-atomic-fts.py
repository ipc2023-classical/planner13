#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run

REVISION = 'db32f471699e'

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/repos/downward/benchmarks')
    suite = DEFAULT_OPTIMAL_SUITE
    environment = BaselSlurmEnvironment(email="silvan.sievers@unibas.ch", export=["PATH"])

    if is_test_run():
        suite = ['gripper:prob01.pddl', 'depot:p01.pddl', 'mystery:prob07.pddl']
        environment = LocalEnvironment(processes=4)

    configs = {
        IssueConfig('astar-blind-transform-atomic-labelreduction', ["--transform", "transform_merge_and_shrink(label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),run_main_loop=false)", "--search", "astar(blind)"]),
        IssueConfig('astar-blind-transform-atomic-bisim', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),run_main_loop=false)", "--search", "astar(blind)"]),
        IssueConfig('astar-blind-transform-atomic-bisim-labelreduction', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),run_main_loop=false)", "--search", "astar(blind)"]),

        IssueConfig('astar-hmax-transform-atomic-labelreduction', ["--transform", "transform_merge_and_shrink(label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),run_main_loop=false)", "--search", "astar(hmax)"]),
        IssueConfig('astar-hmax-transform-atomic-bisim', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),run_main_loop=false)", "--search", "astar(hmax)"]),
        IssueConfig('astar-hmax-transform-atomic-bisim-labelreduction', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),run_main_loop=false)", "--search", "astar(hmax)"]),

        IssueConfig('lazy-ff-transform-atomic-labelreduction', ["--transform", "transform_merge_and_shrink(label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),run_main_loop=false)", "--search", "lazy_greedy([ff()])"]),
        IssueConfig('lazy-ff-transform-atomic-bisim', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),run_main_loop=false)", "--search", "lazy_greedy([ff()])"]),
        IssueConfig('lazy-ff-transform-atomic-bisim-labelreduction', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),run_main_loop=false)", "--search", "lazy_greedy([ff()])"]),
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

    exp.add_absolute_report_step(attributes=attributes)

    exp.run_steps()

main(revisions=[REVISION])
