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
    'astar-blind',
])
exp.add_fetcher('data/2019-02-18-astar-blind-bisim-eval',filter=[remove_revision],merge=True)
exp.add_fetcher('data/2019-02-18-astar-blind-ownbisim-eval',filter=[remove_revision],merge=True)

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

'astar-blind'

'astar-blind-atomic',
'astar-blind-transform-atomic-labelreduction',
'astar-blind-transform-atomic-bisim-labelreduction',
'astar-blind-transform-full-bisim-labelreduction-dfp100-t900',
'astar-blind-transform-full-bisim-labelreduction-dfp1000-t900',
'astar-blind-transform-full-bisim-labelreduction-dfp10000-t900',
'astar-blind-transform-full-bisim-labelreduction-miasm100-t900',
'astar-blind-transform-full-bisim-labelreduction-miasm1000-t900',
'astar-blind-transform-full-bisim-labelreduction-miasm10000-t900',

'astar-blind-transform-atomic-ownbisim-labelreduction',
'astar-blind-transform-full-ownbisim-labelreduction-dfp100-t900',
'astar-blind-transform-full-ownbisim-labelreduction-dfp1000-t900',
'astar-blind-transform-full-ownbisim-labelreduction-dfp10000-t900',
'astar-blind-transform-full-ownbisim-labelreduction-miasm100-t900',
'astar-blind-transform-full-ownbisim-labelreduction-miasm1000-t900',
'astar-blind-transform-full-ownbisim-labelreduction-miasm10000-t900',

## HTML reports

# exp.add_report(AbsoluteReport(attributes=['coverage']))

class OracleScatterPlotReport(ScatterPlotReport):
    """
    A bad copy of ScatterPlotReport that computes the min over the second and
    third given algorithm.
    """
    def __init__(self, take_first_as_third_algo=False, **kwargs):
        ScatterPlotReport.__init__(self, **kwargs)
        self.take_first_as_third_algo = take_first_as_third_algo

    def _fill_categories(self, runs):
        # We discard the *runs* parameter.
        # Map category names to value tuples
        categories = defaultdict(list)
        for (domain, problem), runs in self.problem_runs.items():
            if self.take_first_as_third_algo:
                if len(runs) != 2:
                    continue
                run1, run2 = runs
                assert (run1['algorithm'] == self.algorithms[0] and
                        run2['algorithm'] == self.algorithms[1])
                val1 = run1.get(self.attribute)
                val2 = run2.get(self.attribute)
                if val1 is None and val2 is None:
                    continue
                if val1 is None:
                    oracle_val = val2
                elif val2 is None:
                    oracle_val = val1
                else:
                    oracle_val = min(val1, val2)
                category = self.get_category(run1, run2)
                categories[category].append((val1, oracle_val))
            else:
                if len(runs) != 3:
                    continue
                run1, run2, run3 = runs
                assert (run1['algorithm'] == self.algorithms[0] and
                        run2['algorithm'] == self.algorithms[1] and
                        run3['algorithm'] == self.algorithms[2])
                val1 = run1.get(self.attribute)
                val2 = run2.get(self.attribute)
                val3 = run3.get(self.attribute)
                if val1 is None and val2 is None and val3 is None:
                    continue
                if val2 is None:
                    oracle_val = val3
                elif val3 is None:
                    oracle_val = val2
                else:
                    oracle_val = min(val2, val3)
                category = self.get_category(run1, run2)
                categories[category].append((val1, oracle_val))
        return categories

    def write(self):
        if (self.take_first_as_third_algo and not len(self.algorithms) == 2) or (not self.take_first_as_third_algo and not len(self.algorithms) == 3):
            logging.critical(
                'Oracle Scatter plots need exactly 2 algorithms if take_first_as_third_algo is true, otherwise 3: %s' % self.algorithms)
        self.xlabel = self.xlabel or self.algorithms[0]
        self.ylabel = 'oracle'

        suffix = '.' + self.output_format
        if not self.outfile.endswith(suffix):
            self.outfile += suffix
        tools.makedirs(os.path.dirname(self.outfile))
        self._write_plot(self.runs.values(), self.outfile)

### Latex reports

## expansion plots for bisim
exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind',
            'astar-blind-transform-atomic-bisim-labelreduction',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-vs-astar-blind-transform-atomic-bisim-labelreduction'),
)

exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind',
            'astar-blind-transform-full-bisim-labelreduction-dfp1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-vs-astar-blind-transform-full-bisim-labelreduction-dfp1000-t900'),
)

exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind',
            'astar-blind-transform-full-bisim-labelreduction-miasm1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-vs-astar-blind-transform-full-bisim-labelreduction-miasm1000-t900'),
)

exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind-transform-atomic-bisim-labelreduction',
            'astar-blind-transform-full-bisim-labelreduction-dfp1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-transform-atomic-bisim-labelreduction-vs-astar-blind-transform-full-bisim-labelreduction-dfp1000-t900'),
)

exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind-transform-atomic-bisim-labelreduction',
            'astar-blind-transform-full-bisim-labelreduction-miasm1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-transform-atomic-bisim-labelreduction-vs-astar-blind-transform-full-bisim-labelreduction-miasm1000-t900'),
)


## expansion plots for ownbisim
exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind',
            'astar-blind-transform-atomic-ownbisim-labelreduction',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-vs-astar-blind-transform-atomic-ownbisim-labelreduction'),
)

exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind',
            'astar-blind-transform-full-ownbisim-labelreduction-dfp1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-vs-astar-blind-transform-full-ownbisim-labelreduction-dfp1000-t900'),
)

exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind',
            'astar-blind-transform-full-ownbisim-labelreduction-miasm1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-vs-astar-blind-transform-full-ownbisim-labelreduction-dfp1000-t900'),
)

exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind-transform-atomic-ownbisim-labelreduction',
            'astar-blind-transform-full-ownbisim-labelreduction-dfp1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-transform-atomic-ownbisim-labelreduction-vs-astar-blind-transform-full-ownbisim-labelreduction-dfp1000-t900'),
)

exp.add_report(
    ScatterPlotReport(
        filter_algorithm=[
            'astar-blind-transform-atomic-ownbisim-labelreduction',
            'astar-blind-transform-full-ownbisim-labelreduction-miasm1000-t900',
        ],
        # get_category=lambda run1, run2: run1['domain'],
        attributes=['expansions_until_last_jump'],
        format='tex',
    ),
    outfile=os.path.join(exp.eval_dir, 'astar-blind-transform-atomic-ownbisim-labelreduction-vs-astar-blind-transform-full-ownbisim-labelreduction-miasm1000-t900'),
)

exp.run_steps()
