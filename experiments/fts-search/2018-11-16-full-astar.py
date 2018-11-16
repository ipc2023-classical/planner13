#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os
import subprocess

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run, get_experiment_name

REVISION = '9f684e3269fd'

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/repos/downward/benchmarks')
    suite = DEFAULT_OPTIMAL_SUITE
    environment = BaselSlurmEnvironment(email="silvan.sievers@unibas.ch", export=["PATH"])

    if is_test_run():
        suite = ['gripper:prob01.pddl', 'depot:p01.pddl', 'mystery:prob07.pddl']
        environment = LocalEnvironment(processes=4)

    configs = {
        IssueConfig('astar-blind-transform-full-bisim-labelreduction-dfp1k-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),
        IssueConfig('astar-blind-transform-full-bisim-labelreduction-miasm1k-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=1000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(blind)"]),

        IssueConfig('astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1k-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(merge_and_shrink(shrink_strategy=mas_shrink_bisimulation(greedy=false),merge_strategy=mas_merge_stateless(merge_selector=mas_score_based_filtering(scoring_functions=[mas_goal_relevance,mas_dfp,mas_total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=mas_exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))"]),
        IssueConfig('astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1k-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=1000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(merge_and_shrink(shrink_strategy=mas_shrink_bisimulation(greedy=false),merge_strategy=mas_merge_stateless(merge_selector=mas_score_based_filtering(scoring_functions=[mas_goal_relevance,mas_dfp,mas_total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=mas_exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))"]),

        IssueConfig('astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1k-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),goal_relevance,dfp,total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(merge_and_shrink(shrink_strategy=mas_shrink_bisimulation(greedy=false),merge_strategy=mas_merge_stateless(merge_selector=mas_score_based_filtering(scoring_functions=[mas_sf_miasm(shrink_strategy=mas_shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),mas_total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=mas_exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))"]),
        IssueConfig('astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1k-t900', ["--transform", "transform_merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_stateless(merge_selector=score_based_filtering(scoring_functions=[product_size(1000),sf_miasm(shrink_strategy=shrink_bisimulation(greedy=false),max_states=1000,threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=exact(atomic_fts=true,before_shrinking=true,before_merging=false),shrink_atomic_fts=true,run_main_loop=true,max_time=900)", "--search", "astar(merge_and_shrink(shrink_strategy=mas_shrink_bisimulation(greedy=false),merge_strategy=mas_merge_stateless(merge_selector=mas_score_based_filtering(scoring_functions=[mas_sf_miasm(shrink_strategy=mas_shrink_bisimulation(greedy=false),max_states=50000,threshold_before_merge=1),mas_total_order(atomic_ts_order=reverse_level,product_ts_order=new_to_old,atomic_before_product=false)])),label_reduction=mas_exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1))"]),
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
        '{}-astar-blind-transform-full-bisim-labelreduction-dfp1k-t900'.format(REVISION),
        '{}-astar-blind-transform-full-bisim-labelreduction-miasm1k-t900'.format(REVISION),
        '{}-astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1k-t900'.format(REVISION),
        '{}-astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1k-t900'.format(REVISION),
        '{}-astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1k-t900'.format(REVISION),
        '{}-astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1k-t900'.format(REVISION),
    ])

    BASELINE_REV = 'fts-search-base'
    exp.add_fetcher('data/2018-11-05-regular-baselines-eval', filter_algorithm=[
        '{}-astar-blind'.format(BASELINE_REV),
    ],merge=True)

    BASELINE_REV_MAS = 'fts-search-base-mas'
    exp.add_fetcher('data/2018-11-15-baseline-mas-eval', filter_algorithm=[
        '{}-astar-masdfpbisim50k'.format(BASELINE_REV_MAS),
        '{}-astar-masmiasmbisim50k'.format(BASELINE_REV_MAS),
    ],merge=True)

    outfile = os.path.join(exp.eval_dir, get_experiment_name() + '-regular-vs-fts.html')
    exp.add_report(
        ComparativeReport(
            algorithm_pairs=[
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-dfp1k-t900'.format(REVISION)),
                ('{}-astar-blind'.format(BASELINE_REV), '{}-astar-blind-transform-full-bisim-labelreduction-miasm1k-t900'.format(REVISION)),
                ('{}-astar-masdfpbisim50k'.format(BASELINE_REV_MAS), '{}-astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1k-t900'.format(REVISION)),
                ('{}-astar-masdfpbisim50k'.format(BASELINE_REV_MAS), '{}-astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1k-t900'.format(REVISION)),
                ('{}-astar-masmiasmbisim50k'.format(BASELINE_REV_MAS), '{}-astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1k-t900'.format(REVISION)),
                ('{}-astar-masmiasmbisim50k'.format(BASELINE_REV_MAS), '{}-astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1k-t900'.format(REVISION)),
            ],
            format='html',
            attributes=attributes,
        ),
        outfile=outfile,
    )
    exp.add_step('publish-{}'.format(outfile), subprocess.call, ['publish', outfile])

    exp.run_steps()

main(revisions=[REVISION])
