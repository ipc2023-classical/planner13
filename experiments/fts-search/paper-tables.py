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

exp = FastDownwardExperiment()

REVISIONS = [
    'fts-search-base', # 11-05 baseline astar-blind/hmax, baseline lama-first, baseline lazygreedy-ff*,
    'fts-search-base-mas', # 11-15 baseline astar-mas
    '01551daf787f', # 11-19 astar-blind/hmax/mas
    '865a7642e263', # 11-21 lazygreedy-ff*
]

def remove_revision(run):
    algo = run['algorithm']
    for rev in REVISIONS:
        algo = algo.replace('{}-'.format(rev), '')
    run['algorithm'] = algo
    return run

exp.add_fetcher('data/2018-11-05-regular-baselines-eval',filter=[remove_revision])
exp.add_fetcher('data/2018-11-15-baseline-mas-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-hmax-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-masmiasm-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-masdfp-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-baseline-lazygreedy-ff-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-baseline-lama-first-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-21-lazygreedy-ff-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-21-lazygreedy-ff-remaining-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-21-lazygreedy-ffpref-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2018-11-21-lazygreedy-ffpref-remaining-eval',filter=[remove_revision],merge=True)

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

'lama-first',

## HTML reports

exp.add_report(AbsoluteReport(attributes=['coverage']))

## Latex reports

exp.add_report(
    AbsoluteReport(
        filter_algorithm=[
            'lazy-ff-atomic',
            'lazy-ff-transform-atomic-labelreduction',
            'lazy-ff-transform-atomic-bisimown-labelreduction',
            'lazy-ff-transform-full-bisimown-labelreduction-dfp100-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-dfp1000-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-dfp10000-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-miasm100-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-miasm1000-t900',
            'lazy-ff-transform-full-bisimown-labelreduction-miasm10000-t900',

            'lazy-ffpref-atomic',
            'lazy-ffpref-transform-atomic-labelreduction',
            'lazy-ffpref-transform-atomic-bisimown-labelreduction',
            'lazy-ffpref-transform-full-bisimown-labelreduction-dfp100-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-dfp1000-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-dfp10000-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-miasm100-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-miasm1000-t900',
            'lazy-ffpref-transform-full-bisimown-labelreduction-miasm10000-t900',

            'lazy-ff',
            'lazy-ffpref',
            'lama-first',
        ],
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'lazy-ff-ffpref-ownbisim.tex'),
)

exp.run_steps()
