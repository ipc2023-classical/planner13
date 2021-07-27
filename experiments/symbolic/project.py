from collections import namedtuple
import getpass
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import os

from downward.reports.absolute import AbsoluteReport
from downward.reports.scatter import ScatterPlotReport
from lab import tools
from lab.environments import BaselSlurmEnvironment, LocalEnvironment
from lab.experiment import ARGPARSER
from lab.reports import Attribute, geometric_mean

from downward.experiment import FastDownwardExperiment

# Silence import-unused messages. Experiment scripts may use these imports.
assert BaselSlurmEnvironment and LocalEnvironment and ScatterPlotReport


DIR = Path(__file__).resolve().parent
NODE = platform.node()


REMOTE = NODE.startswith("a512")

def parse_args():
    ARGPARSER.add_argument("--tex", action="store_true", help="produce LaTeX output")
    ARGPARSER.add_argument(
        "--relative", action="store_true", help="make relative scatter plots"
    )
    return ARGPARSER.parse_args()


ARGS = parse_args()
TEX = ARGS.tex
RELATIVE = ARGS.relative

EVALUATIONS_PER_TIME = Attribute(
    "evaluations_per_time", min_wins=False, function=geometric_mean, digits=1
)

def get_repo_base() -> Path:
    """Get base directory of the repository, as an absolute path.

    Search upwards in the directory tree from the main script until a
    directory with a subdirectory named ".git" or ".hg" is found.

    Abort if the repo base cannot be found."""
    path = Path(tools.get_script_path())
    while path.parent != path:
        if any((path / d).is_dir() for d in [".git", ".hg"]):
            return path
        path = path.parent
    sys.exit("repo base could not be found")


def remove_file(path: Path):
    try:
        path.unlink()
    except FileNotFoundError:
        pass


def add_evaluations_per_time(run):
    evaluations = run.get("evaluations")
    time = run.get("search_time")
    if evaluations is not None and evaluations >= 100 and time:
        run["evaluations_per_time"] = evaluations / time
    return run


def _get_exp_dir_relative_to_repos_dir():
    repo_name = get_repo_base().name
    script = Path(tools.get_script_path())
    project = script.parent.name
    expname = script.stem
    return Path(repo_name) / "experiments" / project / "data" / expname


def add_scp_step(exp):
    remote_exp = Path(USER.remote_repos) / _get_exp_dir_relative_to_repos_dir()
    exp.add_step(
        "scp-eval-dir",
        subprocess.call,
        [
            "scp",
            "-r",  # Copy recursively.
            "-C",  # Compress files.
            f"{USER.scp_login}:{remote_exp}-eval",
            f"{exp.path}-eval",
        ],
    )


def fetch_algorithm(exp, expname, algo, *, new_algo=None):
    """Fetch (and possibly rename) a single algorithm from *expname*."""
    new_algo = new_algo or algo

    def rename_and_filter(run):
        if run["algorithm"] == algo:
            run["algorithm"] = new_algo
            run["id"][0] = new_algo
            return run
        return False

    exp.add_fetcher(
        f"data/{expname}-eval",
        filter=rename_and_filter,
        name=f"fetch-{new_algo}-from-{expname}",
        merge=True,
    )


def add_absolute_report(exp, *, name=None, outfile=None, **kwargs):
    report = AbsoluteReport(**kwargs)
    if name and not outfile:
        outfile = f"{name}.{report.output_format}"
    elif outfile and not name:
        name = Path(outfile).name
    elif not name and not outfile:
        name = f"{exp.name}-abs"
        outfile = f"{name}.{report.output_format}"

    if not Path(outfile).is_absolute():
        outfile = Path(exp.eval_dir) / outfile

    exp.add_report(report, name=name, outfile=outfile)
    if not REMOTE:
        exp.add_step(f"open-{name}", subprocess.call, ["xdg-open", outfile])
    # exp.add_step(f"publish-{name}", subprocess.call, ["publish", outfile])


class CommonExperiment(FastDownwardExperiment):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.add_step("build", self.build)
        self.add_step("start", self.start_runs)
        self.add_fetcher(name="fetch")

#        if not REMOTE:
#            self.add_step(
#                "remove-eval-dir", shutil.rmtree, self.eval_dir, ignore_errors=True
#            )
#            add_scp_step(self)

        if REMOTE:
            self.add_step("compress-runs", subprocess.call, ["tar", "-C", f"{self.path}",  "--exclude", "code*", "-zcvf", f"/nfs/home/cs.aau.dk/bx56lg/{self.name}.tar.gz", "."] )


        self.add_parser(self.EXITCODE_PARSER)
        self.add_parser(self.TRANSLATOR_PARSER)
        self.add_parser(self.SINGLE_SEARCH_PARSER)
        self.add_parser(self.PLANNER_PARSER)
        self.add_parser(DIR / "parser.py")
        self.add_parser(DIR / "parser_symbolic.py")
        self.add_parse_again_step()


class ReportExperiment(FastDownwardExperiment):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        for d in os.listdir('data'):
            if  not d.endswith("-eval"):  # d.startswith("exp") and
                self.add_fetcher(f"data/{d}", name=f"fetch-{d}", merge=True)

#        if not REMOTE:
#            self.add_step(
#                "remove-eval-dir", shutil.rmtree, self.eval_dir, ignore_errors=True
#            )
#            add_scp_step(self)

        if REMOTE:
            self.add_step("compress-runs", subprocess.call, ["tar", "--exclude", "code*", "-zcvf", f"/scratch/alto/{self.name}.tar.gz", f"{self.path}"] )


        self.add_parser(self.EXITCODE_PARSER)
        self.add_parser(self.TRANSLATOR_PARSER)
        self.add_parser(self.SINGLE_SEARCH_PARSER)
        self.add_parser(self.PLANNER_PARSER)
        self.add_parser(DIR / "parser.py")
        # self.add_parse_again_step()



DEFAULT_OPTIMAL_SUITE = [
    'agricola-opt18-strips', 'airport', 'barman-opt11-strips',
    'barman-opt14-strips', 'blocks', 'childsnack-opt14-strips',
    'data-network-opt18-strips', 'depot', 'driverlog',
    'elevators-opt08-strips', 'elevators-opt11-strips',
    'floortile-opt11-strips', 'floortile-opt14-strips', 'freecell',
    'ged-opt14-strips', 'grid', 'gripper', 'hiking-opt14-strips',
    'logistics00', 'logistics98', 'miconic', 'movie', 'mprime',
    'mystery', 'nomystery-opt11-strips', 'openstacks-opt08-strips',
    'openstacks-opt11-strips', 'openstacks-opt14-strips',
    'openstacks-strips', 'organic-synthesis-opt18-strips',
    'organic-synthesis-split-opt18-strips', 'parcprinter-08-strips',
    'parcprinter-opt11-strips', 'parking-opt11-strips',
    'parking-opt14-strips', 'pegsol-08-strips',
    'pegsol-opt11-strips', 'petri-net-alignment-opt18-strips',
    'pipesworld-notankage', 'pipesworld-tankage', 'psr-small', 'rovers',
    'satellite', 'scanalyzer-08-strips', 'scanalyzer-opt11-strips',
    'snake-opt18-strips', 'sokoban-opt08-strips',
    'sokoban-opt11-strips', 'spider-opt18-strips', 'storage',
    'termes-opt18-strips', 'tetris-opt14-strips',
    'tidybot-opt11-strips', 'tidybot-opt14-strips', 'tpp',
    'transport-opt08-strips', 'transport-opt11-strips',
    'transport-opt14-strips', 'trucks-strips', 'visitall-opt11-strips',
    'visitall-opt14-strips', 'woodworking-opt08-strips',
    'woodworking-opt11-strips', 'zenotravel']

DEFAULT_SATISFICING_SUITE = [
    'agricola-sat18-strips', 'airport', 'assembly',
    'barman-sat11-strips', 'barman-sat14-strips', 'blocks',
    'caldera-sat18-adl', 'caldera-split-sat18-adl', 'cavediving-14-adl',
    'childsnack-sat14-strips', 'citycar-sat14-adl',
    'data-network-sat18-strips', 'depot', 'driverlog',
    'elevators-sat08-strips', 'elevators-sat11-strips',
    'flashfill-sat18-adl', 'floortile-sat11-strips',
    'floortile-sat14-strips', 'freecell', 'ged-sat14-strips', 'grid',
    'gripper', 'hiking-sat14-strips', 'logistics00', 'logistics98',
    'maintenance-sat14-adl', 'miconic', 'miconic-fulladl',
    'miconic-simpleadl', 'movie', 'mprime', 'mystery',
    'nomystery-sat11-strips', 'nurikabe-sat18-adl', 'openstacks',
    'openstacks-sat08-adl', 'openstacks-sat08-strips',
    'openstacks-sat11-strips', 'openstacks-sat14-strips',
    'openstacks-strips', 'optical-telegraphs',
    'organic-synthesis-sat18-strips',
    'organic-synthesis-split-sat18-strips', 'parcprinter-08-strips',
    'parcprinter-sat11-strips', 'parking-sat11-strips',
    'parking-sat14-strips', 'pathways',
    'pegsol-08-strips', 'pegsol-sat11-strips', 'philosophers',
    'pipesworld-notankage', 'pipesworld-tankage', 'psr-large',
    'psr-middle', 'psr-small', 'rovers', 'satellite',
    'scanalyzer-08-strips', 'scanalyzer-sat11-strips', 'schedule',
    'settlers-sat18-adl', 'snake-sat18-strips', 'sokoban-sat08-strips',
    'sokoban-sat11-strips', 'spider-sat18-strips', 'storage',
    'termes-sat18-strips', 'tetris-sat14-strips',
    'thoughtful-sat14-strips', 'tidybot-sat11-strips', 'tpp',
    'transport-sat08-strips', 'transport-sat11-strips',
    'transport-sat14-strips', 'trucks', 'trucks-strips',
    'visitall-sat11-strips', 'visitall-sat14-strips',
    'woodworking-sat08-strips', 'woodworking-sat11-strips',
    'zenotravel']


DEFAULT_TEST_SUITE = ["depot:p01.pddl", "gripper:prob01.pddl"]
