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

def rename_domain(run):
    algo = run['algorithm']
    domain = run['domain']
    problem = run['problem']
    assert run['id'] == [algo, domain, problem]

    new_domain = domain.replace('opt', '')
    new_domain = new_domain.replace('sat', '')
    run['domain'] = new_domain

    new_id = [algo, new_domain, problem]
    run['id'] = new_id

    return run

exp.add_fetcher('data/2018-11-05-regular-baselines-eval',filter=[rename_domain,remove_revision])
exp.add_fetcher('data/2018-11-15-baseline-mas-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-hmax-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-masmiasm-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-astar-masdfp-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-baseline-lazygreedy-ff-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-19-baseline-lama-first-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-21-lazygreedy-ff-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-21-lazygreedy-ff-remaining-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-21-lazygreedy-ffpref-eval',filter=[rename_domain,remove_revision],merge=True)
exp.add_fetcher('data/2018-11-21-lazygreedy-ffpref-remaining-eval',filter=[rename_domain,remove_revision],merge=True)

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

'mas-astar-masdfpbisim50k',
'astar-masdfpbisim50k-atomic',
'astar-masdfpbisim50k-transform-atomic-labelreduction',
'astar-masdfpbisim50k-transform-atomic-bisim-labelreduction',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',

'mas-astar-masmiasmbisim50k',
'astar-masmiasmbisim50k-atomic',
'astar-masmiasmbisim50k-transform-atomic-labelreduction',
'astar-masmiasmbisim50k-transform-atomic-bisim-labelreduction',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',

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

class PerDomainCoverageReport(PlanningReport):
    def __init__(self, algorithm_blocks, algo_to_print, **kwargs):
        PlanningReport.__init__(self, **kwargs)
        self.algorithm_blocks = algorithm_blocks
        if len(self.attributes) != 1:
            logging.critical('ScatterPlotReport needs exactly one attribute')
        self.attribute = self.attributes[0]
        self.algo_to_print = algo_to_print

    def get_text(self):
        algorithms = []
        for algorithm_block in self.algorithm_blocks:
            assert isinstance(algorithm_block, list)
            algorithms.extend(algorithm_block)
        all_algos = set(run['algorithm'] for run in self.props.values())
        all_algo_strings = [x.encode('ascii') for x in all_algos]
        for algo in algorithms:
            if algo not in all_algo_strings:
                print "{} is not in the data set!".format(algo)
                print "known algorithms: {}".format(sorted(all_algo_strings))
                print "given algorithms: {}".format(sorted(algorithms))
                exit(0)

        domains = set()
        domain_and_algorithm_to_coverage = defaultdict(int)
        for (domain, problem), runs in self.problem_runs.items():
            domains.add(domain)
            for run in runs:
                domain_and_algorithm_to_coverage[(run["domain"], run['algorithm'])] += run["coverage"]

        def turn_row_values_into_string(row_values):
            result = ''
            for block_index, block_values in enumerate(row_values):
                best_value = 0
                for value in block_values:
                    best_value = max(best_value, value)

                block_result = ''
                for index, value in enumerate(block_values):
                    if value == best_value:
                        block_result += '\\textbf{{{}}}'.format(value)
                    else:
                        block_result += '{}'.format(value)
                    if index == len(block_values) - 1 and block_index == len(row_values) - 1:
                        block_result += ' \\\\'
                    else:
                        block_result += ' & '
                result += block_result
            return result

        def turn_list_into_table_row(line):
            result = ''
            for index, value in enumerate(line):
                result += '{}'.format(value)
                if index == len(line) - 1:
                    result += ' \\\\'
                else:
                    result += ' & '
            return result

        def format_algo(algo):
            if algo in self.algo_to_print:
                return self.algo_to_print[algo]
            return algo

        lines = []

        header_line = ['Algo']
        total_coverage_values = []
        for algorithm_block in self.algorithm_blocks:
            block_values = []
            for algorithm in algorithm_block:
                header_line.append(format_algo(algorithm))
                block_values.append(0)
            total_coverage_values.append(block_values)
        lines.append(turn_list_into_table_row(header_line))

        for domain in sorted(domains):
            domain_coverage_values = []
            for block_index, algorithm_block in enumerate(self.algorithm_blocks):
                block_coverage_values = []
                for algo_index, algorithm in enumerate(algorithm_block):
                    coverage = domain_and_algorithm_to_coverage[(domain, algorithm)]
                    block_coverage_values.append(coverage)
                    total_coverage_values[block_index][algo_index] += coverage
                domain_coverage_values.append(block_coverage_values)
            domain_line = turn_row_values_into_string(domain_coverage_values)
            domain_string = domain.replace('-strips', '')
            lines.append('{} & {}'.format(domain_string, domain_line))

        lines.append('\\midrule')
        summary_line = 'Sum & {}'.format(turn_row_values_into_string(total_coverage_values))
        lines.append(summary_line)
        return '\n'.join(lines)

exp.add_report(
    PerDomainCoverageReport(
        algorithm_blocks=[
            [
                # 'astar-hmax',
                'astar-hmax-atomic',
                'astar-hmax-transform-atomic-labelreduction',
                'astar-hmax-transform-atomic-bisim-labelreduction',
                'astar-hmax-transform-full-bisim-labelreduction-dfp100-t900',
                'astar-hmax-transform-full-bisim-labelreduction-dfp1000-t900',
                # 'astar-hmax-transform-full-bisim-labelreduction-dfp10000-t900',
                'astar-hmax-transform-full-bisim-labelreduction-miasm100-t900',
                'astar-hmax-transform-full-bisim-labelreduction-miasm1000-t900',
                # 'astar-hmax-transform-full-bisim-labelreduction-miasm10000-t900',
            ],
            [
                # 'astar-masdfpbisim50k',
                'astar-masdfpbisim50k-atomic',
                'astar-masdfpbisim50k-transform-atomic-labelreduction',
                'astar-masdfpbisim50k-transform-atomic-bisim-labelreduction',
                'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
                'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
                # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
                'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
                'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
                # 'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',
            ],
            [
                # 'astar-masmiasmbisim50k',
                'astar-masmiasmbisim50k-atomic',
                'astar-masmiasmbisim50k-transform-atomic-labelreduction',
                'astar-masmiasmbisim50k-transform-atomic-bisim-labelreduction',
                'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp100-t900',
                'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1000-t900',
                # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp10000-t900',
                'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm100-t900',
                'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1000-t900',
                # 'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm10000-t900',
            ],
            [
                # 'lazy-ff',
                'lazy-ff-atomic',
                'lazy-ff-transform-atomic-labelreduction',
                'lazy-ff-transform-atomic-bisimown-labelreduction',
                'lazy-ff-transform-full-bisimown-labelreduction-dfp100-t900',
                'lazy-ff-transform-full-bisimown-labelreduction-dfp1000-t900',
                # 'lazy-ff-transform-full-bisimown-labelreduction-dfp10000-t900',
                'lazy-ff-transform-full-bisimown-labelreduction-miasm100-t900',
                'lazy-ff-transform-full-bisimown-labelreduction-miasm1000-t900',
                # 'lazy-ff-transform-full-bisimown-labelreduction-miasm10000-t900',
            ],
            [
                # 'lazy-ffpref',
                'lazy-ffpref-atomic',
                'lazy-ffpref-transform-atomic-labelreduction',
                'lazy-ffpref-transform-atomic-bisimown-labelreduction',
                'lazy-ffpref-transform-full-bisimown-labelreduction-dfp100-t900',
                'lazy-ffpref-transform-full-bisimown-labelreduction-dfp1000-t900',
                # 'lazy-ffpref-transform-full-bisimown-labelreduction-dfp10000-t900',
                'lazy-ffpref-transform-full-bisimown-labelreduction-miasm100-t900',
                'lazy-ffpref-transform-full-bisimown-labelreduction-miasm1000-t900',
                # 'lazy-ffpref-transform-full-bisimown-labelreduction-miasm10000-t900',
            ],
            # 'lama-first',
        ],
        algo_to_print={},
        format='tex',
        # attributes=attributes,
        attributes=['coverage'],
    ),
    outfile=os.path.join(exp.eval_dir, 'coverage.tex'),
)

algo_to_print = {
    'astar-hmax': 'FD',
    'astar-hmax-atomic': 'a',
    'astar-hmax-transform-atomic-bisim-labelreduction': 'a-l-b',
    'astar-hmax-transform-full-bisim-labelreduction-dfp1000-t900': 'f-d',
    'astar-hmax-transform-full-bisim-labelreduction-miasm1000-t900': 'f-m',
    'mas-astar-masdfpbisim50k': 'FD',
    'astar-masdfpbisim50k-atomic': 'a',
    'astar-masdfpbisim50k-transform-atomic-bisim-labelreduction': 'a-l-b',
    'astar-masdfpbisim50k-transform-full-bisim-labelreduction-dfp1000-t900': 'f-d',
    'astar-masdfpbisim50k-transform-full-bisim-labelreduction-miasm1000-t900': 'f-m',
    'mas-astar-masmiasmbisim50k': 'FD',
    'astar-masmiasmbisim50k-atomic': 'a',
    'astar-masmiasmbisim50k-transform-atomic-bisim-labelreduction': 'a-l-b',
    'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-dfp1000-t900': 'f-d',
    'astar-masmiasmbisim50k-transform-full-bisim-labelreduction-miasm1000-t900': 'f-m',
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
    DomainComparisonReport(
        filter_algorithm=[
            'mas-astar-masdfpbisim50k',
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
    DomainComparisonReport(
        filter_algorithm=[
            'mas-astar-masmiasmbisim50k',
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

exp.run_steps()
