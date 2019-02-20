from downward.reports import PlanningReport

from collections import defaultdict

# TODO: this currently uses coverage and sum as the hard-coded attribute and
# aggregation function.
"""Compute the maximum coverage over all algorithms for each domain"""
class OracleReport(PlanningReport):
    def __init__(self, **kwargs):
        PlanningReport.__init__(self, **kwargs)

    def get_text(self):
        domains = set()
        domain_to_oracle = defaultdict(int)
        for (domain, problem), runs in self.problem_runs.items():
            domains.add(domain)
            problem_solved = False
            for run in runs:
                coverage = run['coverage']
                if coverage:
                    problem_solved = True
                    break
            if problem_solved:
                domain_to_oracle[domain] += 1

        def turn_list_into_table_row(line):
            result = ''
            for index, value in enumerate(line):
                result += '{}'.format(value)
                if index == len(line) - 1:
                    result += ' \\\\'
                else:
                    result += ' & '
            return result


        lines = []

        header_line = ['']
        header_line.append('oracle')
        lines.append(turn_list_into_table_row(header_line))

        aggregated_value = 0
        for domain in domains:
            line = [domain]
            value = domain_to_oracle[domain]
            aggregated_value += value
            line.append(value)
            lines.append(turn_list_into_table_row(line))

        aggregated_line = ['sum']
        aggregated_line.append(aggregated_value)
        lines.append(turn_list_into_table_row(aggregated_line))
        return '\n'.join(lines)
