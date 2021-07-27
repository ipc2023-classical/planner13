#! /usr/bin/env python

from lab.parser import Parser
from collections import defaultdict
import re

eval = Parser()

regexps = [re.compile("Num variables: (?P<num_fdr_vars>(\d+)) => (?P<num_binary_vars>(\d+))")]

type_atr = {}

def parse_regexps (content, props):
    for l in content.split("\n"):
        for reg in regexps:
            mx = reg.match(l)
            if mx:
                data = mx.groupdict()
                for item in data:
                    props[item] = type_atr[item](data[item]) if item in type_atr else int(data[item])
                break

eval.add_function(parse_regexps)

eval.parse()
