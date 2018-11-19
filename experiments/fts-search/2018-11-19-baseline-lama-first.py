#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, geometric_mean

from downward.reports.compare import ComparativeReport

from common_setup import IssueConfig, IssueExperiment, DEFAULT_OPTIMAL_SUITE, is_test_run

REVISION = "fts-search-base"

def main(revisions=None):
    benchmarks_dir=os.path.expanduser('~/repos/downward/benchmarks')
    # satisficing_strips
    suite = ['agricola-sat18-strips', 'airport', 'barman-sat11-strips',
    'barman-sat14-strips', 'blocks', 'childsnack-sat14-strips',
    'data-network-sat18-strips', 'depot', 'driverlog',
    'elevators-sat08-strips', 'elevators-sat11-strips',
    'floortile-sat11-strips', 'floortile-sat14-strips', 'freecell',
    'ged-sat14-strips', 'grid', 'gripper', 'hiking-sat14-strips',
    'logistics00', 'logistics98', 'miconic', 'movie', 'mprime', 'mystery',
    'nomystery-sat11-strips', 'openstacks-sat08-strips',
    'openstacks-sat11-strips', 'openstacks-sat14-strips', 'openstacks-strips',
    'organic-synthesis-sat18-strips', 'organic-synthesis-split-sat18-strips',
    'parcprinter-08-strips', 'parcprinter-sat11-strips',
    'parking-sat11-strips', 'parking-sat14-strips', 'pathways-noneg',
    'pegsol-08-strips', 'pegsol-sat11-strips', 'pipesworld-notankage',
    'pipesworld-tankage', 'psr-small', 'rovers', 'satellite',
    'scanalyzer-08-strips', 'scanalyzer-sat11-strips', 'snake-sat18-strips',
    'sokoban-sat08-strips', 'sokoban-sat11-strips', 'spider-sat18-strips',
    'storage', 'termes-sat18-strips', 'tetris-sat14-strips',
    'thoughtful-sat14-strips', 'tidybot-sat11-strips', 'tpp',
    'transport-sat08-strips', 'transport-sat11-strips',
    'transport-sat14-strips', 'trucks-strips', 'visitall-sat11-strips',
    'visitall-sat14-strips', 'woodworking-sat08-strips',
    'woodworking-sat11-strips', 'zenotravel']
    environment = BaselSlurmEnvironment(email="silvan.sievers@unibas.ch", export=["PATH"])

    if is_test_run():
        suite = ['gripper:prob01.pddl', 'depot:p01.pddl', 'mystery:prob07.pddl']
        environment = LocalEnvironment(processes=4)

    configs = {
        IssueConfig('lama-first', [], driver_options=['--alias', 'lama-first']),
    }

    exp = IssueExperiment(
        revisions=revisions,
        configs=configs,
        environment=environment,
    )
    exp.add_suite(benchmarks_dir, suite)

    exp.add_parser(exp.EXITCODE_PARSER)
    exp.add_parser(exp.TRANSLATOR_PARSER)
    exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
    exp.add_parser(exp.PLANNER_PARSER)

    attributes = exp.DEFAULT_TABLE_ATTRIBUTES

    exp.add_step('build', exp.build)
    exp.add_step('start', exp.start_runs)
    exp.add_fetcher(name='fetch')

    exp.add_absolute_report_step(attributes=attributes)

    exp.run_steps()

main(revisions=[REVISION])
