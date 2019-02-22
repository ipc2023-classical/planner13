#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import logging
import numpy
import os

from collections import defaultdict

from downward.experiment import FastDownwardExperiment
from downward.reports import PlanningReport
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import ComparativeReport
from downward.reports.scatter import ScatterPlotReport

from lab.reports import Attribute, geometric_mean

from lab import tools

from common_setup import IssueExperiment

from domain_comparison import DomainComparisonReport
from oracle import OracleReport

exp = FastDownwardExperiment()

REVISIONS = [
    'fts-search-base-v2',
    'fcf42e988494',
]

def remove_revision(run):
    algo = run['algorithm']
    for rev in REVISIONS:
        algo = algo.replace('{}-'.format(rev), '')
    run['algorithm'] = algo
    return run

exp.add_fetcher('data/2019-02-18-lazygreedy-baseline-eval',filter=[remove_revision])
exp.add_fetcher('data/2019-02-18-lazygreedy-ff-eval',filter=[remove_revision])
exp.add_fetcher('data/2019-02-18-lazygreedy-ffpref-eval',filter=[remove_revision])

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

attributes = list(IssueExperiment.DEFAULT_TABLE_ATTRIBUTES)
attributes.extend(extra_attributes)

## Configs

all_configs = [
'lazy-ff',
'lazy-ff-atomic',
'lazy-ff-transform-atomic-labelreduction',
'lazy-ff-transform-atomic-bisimown-labelreduction',
'lazy-ff-transform-full-bisimown-labelreduction-dfp100-t900',
'lazy-ff-transform-full-bisimown-labelreduction-dfp1000-t900',
'lazy-ff-transform-full-bisimown-labelreduction-dfp10000-t900',
'lazy-ff-transform-full-bisimown-labelreduction-miasm100-t900',
'lazy-ff-transform-full-bisimown-labelreduction-miasm1000-t900',
'lazy-ff-transform-full-bisimown-labelreduction-miasm10000-t900',

'lazy-ffpref',
'lazy-ffpref-atomic',
'lazy-ffpref-transform-atomic-labelreduction',
'lazy-ffpref-transform-atomic-bisimown-labelreduction',
'lazy-ffpref-transform-full-bisimown-labelreduction-dfp100-t900',
'lazy-ffpref-transform-full-bisimown-labelreduction-dfp1000-t900',
'lazy-ffpref-transform-full-bisimown-labelreduction-dfp10000-t900',
'lazy-ffpref-transform-full-bisimown-labelreduction-miasm100-t900',
'lazy-ffpref-transform-full-bisimown-labelreduction-miasm1000-t900',
'lazy-ffpref-transform-full-bisimown-labelreduction-miasm10000-t900',
]

## HTML reports

exp.add_report(AbsoluteReport(attributes=['coverage'],filter_algorithm=all_configs))

## Latex reports

algo_to_print = {
    'lazy-ff': 'FD',
    'lazy-ff-atomic': 'a',
    'lazy-ff-transform-atomic-bisimown-labelreduction': 'a-l-o',
    'lazy-ff-transform-full-bisimown-labelreduction-dfp1000-t900': 'f-d',
    'lazy-ff-transform-full-bisimown-labelreduction-miasm1000-t900': 'f-m',
    'lazy-ffpref': 'FD',
    'lazy-ffpref-atomic': 'a',
    'lazy-ffpref-transform-atomic-bisimown-labelreduction': 'a-l-o',
    'lazy-ffpref-transform-full-bisimown-labelreduction-dfp1000-t900': 'f-d',
    'lazy-ffpref-transform-full-bisimown-labelreduction-miasm1000-t900': 'f-m',
}

exp.add_report(
    DomainComparisonReport(
        filter_algorithm=[
            'lazy-ff',
            'lazy-ff-atomic',
            # 'lazy-ff-transform-atomic-labelreduction',
            'lazy-ff-transform-atomic-bisimown-labelreduction',
            # 'lazy-ff-transform-full-bisimown-labelreduction-dfp100-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-dfp1000-t900',
            # 'lazy-ff-transform-full-bisimown-labelreduction-dfp10000-t900',
            # 'lazy-ff-transform-full-bisimown-labelreduction-miasm100-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-miasm1000-t900',
            # 'lazy-ff-transform-full-bisimown-labelreduction-miasm10000-t900',
        ],
        algo_to_print=algo_to_print,
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'domain-comparison-coverage-lazy-ff.tex'),
)

exp.add_report(
    OracleReport(
        filter_algorithm=[
            'lazy-ff-atomic',
            # 'lazy-ff-transform-atomic-labelreduction',
            'lazy-ff-transform-atomic-bisimown-labelreduction',
            # 'lazy-ff-transform-full-bisimown-labelreduction-dfp100-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-dfp1000-t900',
            # 'lazy-ff-transform-full-bisimown-labelreduction-dfp10000-t900',
            # 'lazy-ff-transform-full-bisimown-labelreduction-miasm100-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-miasm1000-t900',
            # 'lazy-ff-transform-full-bisimown-labelreduction-miasm10000-t900',
        ],
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'oracle-coverage-lazy-ff.tex'),
)

exp.add_report(
    DomainComparisonReport(
        filter_algorithm=[
            'lazy-ffpref',
            'lazy-ffpref-atomic',
            # 'lazy-ffpref-transform-atomic-labelreduction',
            'lazy-ffpref-transform-atomic-bisimown-labelreduction',
            # 'lazy-ffpref-transform-full-bisimown-labelreduction-dfp100-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-dfp1000-t900',
            # 'lazy-ffpref-transform-full-bisimown-labelreduction-dfp10000-t900',
            # 'lazy-ffpref-transform-full-bisimown-labelreduction-miasm100-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-miasm1000-t900',
            # 'lazy-ffpref-transform-full-bisimown-labelreduction-miasm10000-t900',
        ],
        algo_to_print=algo_to_print,
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'domain-comparison-coverage-lazy-ffpref.tex'),
)

exp.add_report(
    OracleReport(
        filter_algorithm=[
            'lazy-ffpref',
            'lazy-ffpref-atomic',
            # 'lazy-ffpref-transform-atomic-labelreduction',
            'lazy-ffpref-transform-atomic-bisimown-labelreduction',
            # 'lazy-ffpref-transform-full-bisimown-labelreduction-dfp100-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-dfp1000-t900',
            # 'lazy-ffpref-transform-full-bisimown-labelreduction-dfp10000-t900',
            # 'lazy-ffpref-transform-full-bisimown-labelreduction-miasm100-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-miasm1000-t900',
            # 'lazy-ffpref-transform-full-bisimown-labelreduction-miasm10000-t900',
        ],
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'oracle-coverage-lazy-ffpref.tex'),
)

exp.run_steps()
