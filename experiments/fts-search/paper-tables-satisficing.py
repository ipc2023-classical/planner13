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

from common_setup import IssueExperiment, QualityFilters

from domain_comparison import DomainComparisonReport
from oracle import OracleReport

exp = FastDownwardExperiment()

REVISIONS = [
    'fts-search-base-v2',
    'ab305ba7fa1f',
]

def remove_revision(run):
    algo = run['algorithm']
    for rev in REVISIONS:
        algo = algo.replace('{}-'.format(rev), '')
    run['algorithm'] = algo
    return run

def fix_search_time(props):
    if 'search_time' in props and props['search_time'] == 0:
        props['search_time'] = 0.01
    return props

exp.add_fetcher('data/2019-02-18-lazygreedy-baseline-eval',filter=[remove_revision])
exp.add_fetcher('data/2019-02-26-lazygreedy-ff-unitcost-eval',filter=[remove_revision,fix_search_time],merge=True)
exp.add_fetcher('data/2019-02-26-lazygreedy-ffpref-unitcost-eval',filter=[remove_revision,fix_search_time],merge=True)

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
'lazy-ffpref',

'lazy-ff-transformunit-atomic',
'lazy-ff-transformunit-atomic-labelreduction',
'lazy-ff-transformunit-atomic-bisimown-labelreduction',
'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp100-t900',
'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp1000-t900',
'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp10000-t900',
'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm100-t900',
'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm1000-t900',
'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm10000-t900',

'lazy-ffpref-transformunitcost-atomic',
'lazy-ffpref-transformunitcost-atomic-labelreduction',
'lazy-ffpref-transformunitcost-atomic-weakbisim-labelreduction',
'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp100-t900',
'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp1000-t900',
'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp10000-t900',
'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm100-t900',
'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm1000-t900',
'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm10000-t900',
]

## HTML reports

exp.add_report(AbsoluteReport(attributes=['coverage'],filter_algorithm=all_configs))

## Latex reports

algo_to_print = {
    'lazy-ff': '\\SAS',
    'lazy-ff-transformunit-atomic': '\\atomic',
    'lazy-ff-transformunit-atomic-weakbisim-labelreduction': '\\atomicshrink',
    'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp1000-t900': '\\fulldfp',
    'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm1000-t900': '\\fullmiasm',
    'lazy-ffpref-transformunitcost-atomic': '\\atomic',
    'lazy-ffpref-transformunitcost-atomic-weakbisim-labelreduction': '\\atomicshrink',
    'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp1000-t900': '\\fulldfp',
    'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm1000-t900': '\\fullmiasm',
}

exp.add_report(
    DomainComparisonReport(
        filter_algorithm=[
            'lazy-ff',
            'lazy-ff-transformunit-atomic',
            # 'lazy-ff-transformunit-atomic-labelreduction',
            'lazy-ff-transformunit-atomic-weakbisim-labelreduction',
            # 'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp100-t900',
            'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp1000-t900',
            # 'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp10000-t900',
            # 'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm100-t900',
            'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm1000-t900',
            # 'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm10000-t900',
        ],
        algo_to_print=algo_to_print,
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'domain-comparison-coverage-lazy-ff-transformunit.tex'),
)

exp.add_report(
    OracleReport(
        filter_algorithm=[
            'lazy-ff-transformunit-atomic',
            # 'lazy-ff-transformunit-atomic-labelreduction',
            'lazy-ff-transformunit-atomic-weakbisim-labelreduction',
            # 'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp100-t900',
            'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp1000-t900',
            # 'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp10000-t900',
            # 'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm100-t900',
            'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm1000-t900',
            # 'lazy-ff-transformunit-full-weakbisim-labelreduction-miasm10000-t900',
        ],
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'oracle-coverage-lazy-ff-transformunit.tex'),
)

exp.add_report(
    DomainComparisonReport(
        filter_algorithm=[
            'lazy-ffpref',
            'lazy-ffpref-transformunitcost-atomic',
            # 'lazy-ffpref-transformunitcost-atomic-labelreduction',
            'lazy-ffpref-transformunitcost-atomic-weakbisim-labelreduction',
            # 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp100-t900',
            'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp1000-t900',
            # 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp10000-t900',
            # 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm100-t900',
            'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm1000-t900',
            # 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm10000-t900',
        ],
        algo_to_print=algo_to_print,
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'domain-comparison-coverage-lazy-ffpref-transformunitcost.tex'),
)

exp.add_report(
    OracleReport(
        filter_algorithm=[
            'lazy-ffpref',
            'lazy-ffpref-transformunitcost-atomic',
            # 'lazy-ffpref-transformunitcost-atomic-labelreduction',
            'lazy-ffpref-transformunitcost-atomic-weakbisim-labelreduction',
            # 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp100-t900',
            'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp1000-t900',
            # 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp10000-t900',
            # 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm100-t900',
            'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm1000-t900',
            # 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-miasm10000-t900',
        ],
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'oracle-coverage-lazy-ffpref-transformunitcost.tex'),
)

# plots

quality_filters = QualityFilters()

comparison_algo_pairs = [
    ('lazy-ff', 'lazy-ff-transformunit-atomic'),
    ('lazy-ff', 'lazy-ff-transformunit-atomic-weakbisim-labelreduction'),
    ('lazy-ff', 'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp1000-t900'),
    ('lazy-ff-transformunit-atomic', 'lazy-ff-transformunit-atomic-weakbisim-labelreduction'),
    ('lazy-ff-transformunit-atomic', 'lazy-ff-transformunit-full-weakbisim-labelreduction-dfp1000-t900'),

    ('lazy-ffpref', 'lazy-ffpref-transformunitcost-atomic'),
    ('lazy-ffpref', 'lazy-ffpref-transformunitcost-atomic-weakbisim-labelreduction'),
    ('lazy-ffpref', 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp1000-t900'),
    ('lazy-ffpref-transformunitcost-atomic', 'lazy-ffpref-transformunitcost-atomic-weakbisim-labelreduction'),
    ('lazy-ffpref-transformunitcost-atomic', 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp1000-t900'),
]

tex_comparison_algo_pairs = [
    ('lazy-ffpref', 'lazy-ffpref-transformunitcost-atomic'),
    ('lazy-ffpref-transformunitcost-atomic', 'lazy-ffpref-transformunitcost-atomic-weakbisim-labelreduction'),
    ('lazy-ffpref-transformunitcost-atomic-weakbisim-labelreduction', 'lazy-ffpref-transformunitcost-full-weakbisim-labelreduction-dfp1000-t900'),
]

comparison_attributes = [
    'expansions',
    'search_time',
    'total_time',
    # 'quality',
]

step_name = "make-absolute-scatter-plots"
scatter_dir = os.path.join(exp.eval_dir, "scatter-plots")
def make_scatter_plot(algo1, algo2, attribute):
    name = "-".join([attribute, algo1, 'vs', algo2])
    print "Make scatter plots for", name

    report = ScatterPlotReport(
        filter_algorithm=[algo1, algo2],
        filter=[quality_filters.store_costs, quality_filters.add_quality],
        attributes=[attribute],
        format='tex',
    )
    report(
        exp.eval_dir,
        os.path.join(scatter_dir, name))

def make_scatter_plots():
    for algo_pair in tex_comparison_algo_pairs:
        for attribute in comparison_attributes:
            make_scatter_plot(algo_pair[0], algo_pair[1], attribute)

exp.add_step(step_name, make_scatter_plots)

exp.run_steps()
