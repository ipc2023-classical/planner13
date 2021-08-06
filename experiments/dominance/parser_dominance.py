#! /usr/bin/env python

from lab.parser import Parser
from collections import defaultdict
import re

eval = Parser()

regexps = [re.compile("Compute LDSim on (?P<lts_num>(\d+)) LTSs. Total size: (?P<lts_total_size>(\d+)) Total trsize: (?P<lts_total_trsize>(\d+)) Max size: (?P<lts_max_size>(\d+)) Max trsize: (?P<lts_max_trsize>(\d+))"),
           re.compile(".*Init LDSim in (?P<time_init_ldsim>(.*))s:.*"),
           re.compile("LDSim computed (?P<time_ldsim>(.*))"),
           re.compile("Dead operators due to dead labels: (?P<dead_ops_by_labels>(\d+)) / (?P<orig_ops>(\d+)) \((?P<perc_dead_ops_by_labels>(\d*[.\d+]*))\%\)"),
           re.compile("Dead operators detected by storing original operators: (?P<dead_ops_by_stored>(\d+)) / (?P<orig_ops>(\d+)) \((?P<perc_dead_ops_by_stored>(\d*[.\d+]*))\%\)"),
           re.compile("Simulation pruning (?P<pruning_desactivated>(.*)): (?P<pruned_desactivated>(\d+)) pruned (?P<checked_desactivated>(\d+)) checked (?P<inserted_desactivated>(\d+)) inserted (?P<deadends_desactivated>(\d+)) deadends"),
           re.compile("Numeric LDSim computed(?P<time_ldsim> (.*))"),
           re.compile("Numeric LDSim outer iterations: (?P<outer_iterations_numeric_ldsimulation>(.*))"),
           re.compile("Numeric LDSim inner iterations: (?P<inner_iterations_numeric_ldsimulation>(.*))"),
           re.compile("First node pruned after checking (?P<dom_checked_before_first_pruned>(\d+)) and inserting (?P<dom_inserted_before_first_pruned>(\d+))"),
           re.compile("Done initializing simulation heuristic \[(?P<total_simulation_time>(.*))s\]"),
           re.compile('Done initializing merge-and-shrink heuristic \[(?P<total_abstraction_time>(.*))s\]'),
           re.compile('Final abstractions: (?P<final_abstractions>(\d+))'),
           re.compile('Useless vars: (?P<useless_vars>(\d+))'),
           re.compile('Total Simulations: (?P<total_simulations>(\d+))'),
           re.compile('Only Simulations: (?P<only_simulations>(\d+))'),
           re.compile('Similarity equivalences: (?P<similarity_equivalences>(\d+))'),
           re.compile('Completed preprocessing: (?P<time_completed_preprocessing>(.*))'),
           re.compile('Simulations Found in (?P<num_variables_with_positive_dominance>(\d+)) out of (?P<total_num_variables>(\d+)) variables'),
           re.compile('Computed tau labels .*: (?P<tau_labels_all>(\d+)) : (?P<tau_labels_some>(\d+)) / (?P<total_labels>(\d+))')]

type_atr = {'dead_ops_by_labels' : int, 'perc_dead_ops_by_labels' : float, 'orig_ops' : int,
            'dead_ops_by_stored' : int, 'perc_dead_ops_by_stored' : float,
            'lts_num' : int, 'lts_total_size' : int,  'lts_max_size' : int, 'lts_total_trsize' : int, 'lts_max_trsize' : int,
            'pruning_desactivated' : (lambda x : 1 if "desactivated" == x else 0 ),
            'pruned_desactivated' : int, 'checked_desactivated' : int, 'inserted_desactivated' : int, 'deadends_desactivated' : int,
            'time_init_ldsim' : lambda x : max(0.01, float(x)),
            'outer_iterations_numeric_ldsimulation' : int,
            'inner_iterations_numeric_ldsimulation' : int,
            "total_simulation_time" : lambda x : max(0.01, float(x)),
            "total_abstraction_time" : lambda x : max(0.01, float(x)), "final_abstractions" : int,
            "useless_vars" : int, "total_simulations" : int, "only_simulations" : int, "similarity_equivalences" : int,
            'dom_inserted_before_first_pruned' : int,  'dom_checked_before_first_pruned' : int,
            'time_ldsim' : lambda x : max(0.01, float(x)),
            'time_completed_preprocessing' : lambda x : max(0.01, float(x)),
            'total_num_variables' : int, 'num_variables_with_positive_dominance' : int,
            'tau_labels_all' : int, 'tau_labels_some' : int, 'total_labels' : int,
        }

def parse_regexps (content, props):
    for l in content.split("\n"):
        for reg in regexps:
            mx = reg.match(l)
            if mx:
                data = mx.groupdict()
                for item in data:
                    props[item] = type_atr[item](data[item])
                break

    props["did_prune"] = 1 if "dom_checked_before_first_pruned" in props else 0



def parse_numeric_dominance (content, props):
    check = False
    new_var = False
    num_vars = 0
    new_var_geq0 = False
    new_var_geq1 = False
    min_val = 100000000
    max_val = -100000000
    num_variables_with_dominance = 0
    num_variables_with_dominance_geq0 = 0
    num_variables_with_dominance_geq1 = 0

    for l in content.split("\n"):
        if l == "------":
            if not check:
                check = True
            else:
                num_vars += 1
            new_var = True
            new_var_geq0 = True
            new_var_geq1 = True
        elif check:
            if l == "------" or len(l.strip()) == 0:
                continue

            if ":" in l and not "infinity" in l and l.split(":")[0].replace('-','',1).isdigit():
                val = l.split(":")[0]
                if "(" in val:
                    val = val.split("(")[0]
                val = int(val)
                min_val = min(min_val, val)
                max_val = max(max_val, val)

                if new_var:
                    num_variables_with_dominance += 1
                    new_var = False

                if new_var_geq0 and val >= 0:
                    num_variables_with_dominance_geq0 += 1
                    new_var_geq0 = False

                if new_var_geq1 and val >= 1:
                    num_variables_with_dominance_geq1 += 1
                    new_var_geq1 = False
            elif "infinity" not in l:
                props['min_negative_dominance'] = min_val
                props['max_positive_dominance'] = max_val
                props["has_dominance"] = 1 if (max_val > -100000000) else 0
                props["has_positive_dominance"] = 1 if (max_val > 0) else 0
                props["has_negative_dominance"] = 1 if (min_val < 0) and (min_val > -100000000) else 0
                props["has_qualitative_dominance"] = 1 if num_variables_with_dominance_geq0 > 0 else 0
                props["num_variables_with_dominance"] = num_variables_with_dominance
                props["num_variables_with_dominance_geq0"] = num_variables_with_dominance_geq0
                props["num_variables_with_dominance_geq1"] = num_variables_with_dominance_geq1

                props["percentage_variables_with_dominance"] = num_variables_with_dominance/float(num_vars)
                props["percentage_variables_with_dominance_geq0"] = num_variables_with_dominance_geq0/float(num_vars)
                props["percentage_variables_with_dominance_geq1"] = num_variables_with_dominance_geq1/float(num_vars)
                return






def fix_error (content, props):
    if not props["error"].startswith("unexplained"):
        return
    for l in content.split("\n"):
        if l.startswith("Peak memory: Failed to allocate memory. Released memory buffer.") or l.startswith("CUDD: out of memory allocating"):
            props["error"] = "out-of-memory"
            return


eval.add_function(parse_regexps)

eval.add_function(parse_numeric_dominance)

eval.add_function(fix_error)

def parse_layers (content, props):

    regexp = re.compile("f = (?P<f_value>(\d+)) \[(?P<evaluated>(\d+)) evaluated, (?P<expanded>(\d+)) expanded, [(?P<pruned>(\d+)) pruned, ]*t=(?P<time>(.*))s, (?P<peak_memory>(\d+)) KB\]")

    for l in content.split("\n"):

        mx = regexp.match(l)
        if mx:
            data = mx.groupdict()
            if "f_layers" not in props:
                props["f_layers"] = []
            for d in data:
                if d == "time":
                    data[d] = float(data[d])
                else:
                    data[d] = int(data[d])
            props["f_layers"].append(data)


eval.add_function(parse_layers)


eval.parse()
