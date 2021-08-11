#! /usr/bin/env python

import os

import project

import lab
from lab.reports import Attribute

REPO = project.get_repo_base()
ENV = project.LocalEnvironment(processes=2)
exp = project.ReportExperiment(environment=ENV)

project.add_absolute_report(exp,
                            attributes=["error", "cost","total_time","coverage"],
                            name="report-cov"
)


project.add_absolute_report(exp,
                            attributes=["error", "cost","total_time","coverage"],
                            filter_algorithm=['blind', 'def:sfw'],
                            name="report-sfw"
)


project.add_absolute_report(exp,
                            attributes=["error", "cost","total_time","coverage", "expansions_until_last_jump"],
                            name="report-cov-dom", filter_algorithm=['atomiclr-blind','atomiclr-blind-parsucc', 'blind', 'blind-parsucc']
)


for alg1, alg2 in [("blind", "blind-parsucc"), ("blind", "def:sfw") ]:
    for atr in ["total_time", "expansions_until_last_jump"]:
        exp.add_report(
            project.ScatterPlotReport(
                attributes=[atr], filter_algorithm=[alg1, alg2],
            ),
            name=f"compare-{alg1}-vs-{alg2}-{atr}",
        )


exp.run_steps()
