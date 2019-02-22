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

exp.add_fetcher('data/2019-02-18-astar-baseline-eval',filter=[remove_revision],filter_algorithm=[
    'astar-hmax',
    'astar-masdfpbisim50k',
    'astar-masmiasmbisim50k',
])
exp.add_fetcher('data/2019-02-18-astar-hmax-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2019-02-18-astar-masdfp-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2019-02-18-astar-masmiasm-eval',filter=[remove_revision],merge=True)

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
'astar-hmax',
'astar-hmax-atomic',
'astar-hmax-transform-atomic-labelreduction',
'astar-hmax-transform-atomic-bisim-labelreduction',
'astar-hmax-transform-full-bisim-labelreduction-dfp100-t900',
'astar-hmax-transform-full-bisim-labelreduction-dfp1000-t900',
'astar-hmax-transform-full-bisim-labelreduction-dfp10000-t900',
'astar-hmax-transform-full-bisim-labelreduction-miasm100-t900',
'astar-hmax-transform-full-bisim-labelreduction-miasm1000-t900',
'astar-hmax-transform-full-bisim-labelreduction-miasm10000-t900',

'astar-masdfpbisim50k',
'astar-masdfpbisim50k-atomic',
'astar-masdfpbisim50k-transform-atomic-labelreduction',
'astar-masdfpbisim50k-transform-atomic-bisim-labelreduction',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',

'astar-masmiasmbisim50k',
'astar-masmiasmbisim50k-atomic',
'astar-masmiasmbisim50k-transform-atomic-labelreduction',
'astar-masmiasmbisim50k-transform-atomic-bisim-labelreduction',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',
]

## HTML reports

exp.add_report(AbsoluteReport(attributes=['coverage'],filter_algorithm=all_configs))

## Latex reports

algo_to_print = {
    'astar-hmax': 'FD',
    'astar-hmax-atomic': 'a',
    'astar-hmax-transform-atomic-bisim-labelreduction': 'a-l-b',
    'astar-hmax-transform-full-bisim-labelreduction-dfp1000-t900': 'f-d',
    'astar-hmax-transform-full-bisim-labelreduction-miasm1000-t900': 'f-m',
    'astar-masdfpbisim50k': 'FD',
    'astar-masdfpbisim50k-atomic': 'a',
    'astar-masdfpbisim50k-transform-atomic-bisim-labelreduction': 'a-l-b',
    'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1000-t900': 'f-d',
    'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1000-t900': 'f-m',
    'astar-masmiasmbisim50k': 'FD',
    'astar-masmiasmbisim50k-atomic': 'a',
    'astar-masmiasmbisim50k-transform-atomic-bisim-labelreduction': 'a-l-b',
    'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1000-t900': 'f-d',
    'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1000-t900': 'f-m',
}

exp.add_report(
    DomainComparisonReport(
        filter_algorithm=[
            'astar-hmax',
            'astar-hmax-atomic',
            # 'astar-hmax-transform-atomic-labelreduction',
            'astar-hmax-transform-atomic-bisim-labelreduction',
            # 'astar-hmax-transform-full-bisim-labelreduction-dfp100-t900',
            'astar-hmax-transform-full-bisim-labelreduction-dfp1000-t900',
            # 'astar-hmax-transform-full-bisim-labelreduction-dfp10000-t900',
            # 'astar-hmax-transform-full-bisim-labelreduction-miasm100-t900',
            'astar-hmax-transform-full-bisim-labelreduction-miasm1000-t900',
            # 'astar-hmax-transform-full-bisim-labelreduction-miasm10000-t900',
        ],
        algo_to_print=algo_to_print,
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'domain-comparison-coverage-astar-hmax.tex'),
)

exp.add_report(
    OracleReport(
        filter_algorithm=[
            'astar-hmax-atomic',
            # 'astar-hmax-transform-atomic-labelreduction',
            'astar-hmax-transform-atomic-bisim-labelreduction',
            # 'astar-hmax-transform-full-bisim-labelreduction-dfp100-t900',
            'astar-hmax-transform-full-bisim-labelreduction-dfp1000-t900',
            # 'astar-hmax-transform-full-bisim-labelreduction-dfp10000-t900',
            # 'astar-hmax-transform-full-bisim-labelreduction-miasm100-t900',
            'astar-hmax-transform-full-bisim-labelreduction-miasm1000-t900',
            # 'astar-hmax-transform-full-bisim-labelreduction-miasm10000-t900',
        ],
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'oracel-coverage-astar-hmax.tex'),
)

exp.add_report(
    DomainComparisonReport(
        filter_algorithm=[
            'astar-masdfpbisim50k',
            'astar-masdfpbisim50k-atomic',
            # 'astar-masdfpbisim50k-transform-atomic-labelreduction',
            'astar-masdfpbisim50k-transform-atomic-bisim-labelreduction',
            # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
            'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
            # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
            # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
            'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
            # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',
        ],
        algo_to_print=algo_to_print,
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'domain-comparison-coverage-astar-masdfpbisim50k.tex'),
)

exp.add_report(
    OracleReport(
        filter_algorithm=[
            'astar-masdfpbisim50k-atomic',
            # 'astar-masdfpbisim50k-transform-atomic-labelreduction',
            'astar-masdfpbisim50k-transform-atomic-bisim-labelreduction',
            # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
            'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
            # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
            # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
            'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
            # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',
        ],
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'oracle-coverage-astar-masdfpbisim50k.tex'),
)

exp.add_report(
    DomainComparisonReport(
        filter_algorithm=[
            'astar-masmiasmbisim50k',
            'astar-masmiasmbisim50k-atomic',
            # 'astar-masmiasmbisim50k-transform-atomic-labelreduction',
            'astar-masmiasmbisim50k-transform-atomic-bisim-labelreduction',
            # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
            'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
            # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
            # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
            'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
            # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',
        ],
        algo_to_print=algo_to_print,
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'domain-comparison-coverage-astar-masmiasmbisim50k.tex'),
)

exp.add_report(
    OracleReport(
        filter_algorithm=[
            'astar-masmiasmbisim50k-atomic',
            # 'astar-masmiasmbisim50k-transform-atomic-labelreduction',
            'astar-masmiasmbisim50k-transform-atomic-bisim-labelreduction',
            # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
            'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
            # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
            # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
            'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
            # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',
        ],
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'oracle-coverage-astar-masmiasmbisim50k.tex'),
)

exp.run_steps()
