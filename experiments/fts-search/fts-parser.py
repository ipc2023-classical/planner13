#! /usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern('ms_algorithm_time', 'Merge-and-shrink algorithm runtime: (.+)s', required=False, type=float)
parser.add_pattern('ms_atomic_algorithm_time', 'Merge-and-shrink atomic construction runtime: (.+)s', required=False, type=float)
parser.add_pattern('ms_memory_delta', 'Final peak memory increase of merge-and-shrink algorithm: (\d+) KB', required=False, type=int)
parser.add_pattern('fts_transformation_time', 'Transform time: (.+)s', required=False, type=float)
parser.add_pattern('fts_search_task_construction_time', 'Done building search task wrapper for FTS task: (.+)s', required=False, type=float)
parser.add_pattern('fts_plan_reconstruction_time', 'Plan reconstruction time: (.+)s', required=False, type=float)

def check_flags(content, props):
    atomic_task_constructed = False
    for line in content.split('\n'):
        if line == 'Main task constructed':
            atomic_task_constructed = True
    props['atomic_task_constructed'] = atomic_task_constructed

parser.add_function(check_flags)

parser.parse()