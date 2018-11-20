#! /usr/bin/env python
# -*- coding: utf-8 -*-

import itertools
import numpy
import os

from collections import defaultdict

from downward.experiment import FastDownwardExperiment
from downward.reports import PlanningReport
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import ComparativeReport
from downward.reports.comparison import ComparisonReport # custom comparison report
from downward.reports.scatter import ScatterPlotReport

from lab.reports import Attribute, geometric_mean

from common_setup import IssueExperiment

exp = FastDownwardExperiment()

REVISIONS = [
    '01551daf787f', # 11-19 astar-*, lazygreedy-ff, baseline lazygreedy-ff*
    'fts-search-base', # 11-05 baseline astar-blind/hmax
    'fts-search-base-mas', # 11-15 baseline  astar-mas
    'todo', # 11-20 lazygreedy-ff-pref
]

def remove_revision(run):
    algo = run['algorithm']
    for rev in REVISIONS:
        algo = algo.replace('{}-'.format(rev), '')
    run['algorithm'] = algo
    return run

exp.add_fetcher('data/2018-11-05-regular-baselines-eval',filter=[remove_revision])
exp.add_fetcher('data/2018-11-15-baseline-mas-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-blind-bisim-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-blind-ownbisim-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-blind-hmax-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-blind-masmiasm-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-lazygreedy-ff-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-baseline-lazygreedy-ff-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-baseline-lama-first-eval',filter=[remove_revision],merge=True)

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

## HTML reports

# exp.add_report(AbsoluteReport(attributes=attributes))

exp.add_report(
    AbsoluteReport(
        filter_algorithm=[
            'astar-blind-atomic',
            'astar-blind-transform-atomic-labelreduction',
            'astar-blind-transform-atomic-bisim-labelreduction',
            'astar-blind-transform-full-bisim-labelreduction-dfp100-t900',
            'astar-blind-transform-full-bisim-labelreduction-dfp1000-t900',
            'astar-blind-transform-full-bisim-labelreduction-dfp10000-t900',
            'astar-blind-transform-full-bisim-labelreduction-miasm100-t900',
            'astar-blind-transform-full-bisim-labelreduction-miasm1000-t900',
            'astar-blind-transform-full-bisim-labelreduction-miasm10000-t900',
        ],
        format='html',
        attributes=attributes,
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-bisim.html'),
)

exp.add_report(
    ComparativeReport(
        algorithm_pairs=[
            ('astar-blind', 'astar-blind-atomic'),
            ('astar-blind', 'astar-blind-transform-atomic-labelreduction'),
            ('astar-blind', 'astar-blind-transform-atomic-bisim-labelreduction'),
            ('astar-blind', 'astar-blind-transform-full-bisim-labelreduction-dfp100-t900'),
            ('astar-blind', 'astar-blind-transform-full-bisim-labelreduction-dfp1000-t900'),
            ('astar-blind', 'astar-blind-transform-full-bisim-labelreduction-dfp10000-t900'),
            ('astar-blind', 'astar-blind-transform-full-bisim-labelreduction-miasm100-t900'),
            ('astar-blind', 'astar-blind-transform-full-bisim-labelreduction-miasm1000-t900'),
            ('astar-blind', 'astar-blind-transform-full-bisim-labelreduction-miasm10000-t900'),
        ],
        format='html',
        attributes=attributes,
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-bisim-comparison.html'),
)

# some example plot
exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind',
            'astar-blind-transform-full-bisim-labelreduction-miasm1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='png',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-vs-astar-blind-transform-full-bisim-labelreduction-miasm1000-t900.png'),
)

exp.run_steps()
