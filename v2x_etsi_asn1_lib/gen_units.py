#!/usr/bin/env python3
import sys
import re


# This script is used to generate units.h based on the ASN.1 files

def get_last_unit(line):
    last_unit_num = re.search(r'@?unit:? the value is scaled by (\d+|\d+,\d+)$', line, re.I)
    if last_unit_num:
        return 1 / float(last_unit_num.group(1)), ""
    last_unit_num = re.search(r'@?unit:? percent$', line, re.I)
    if last_unit_num:
        return 0.01, ""
    last_unit_num = re.search(r'@?unit:? (\d+|\d+,\d+) *over *([0-9 ,]+)(.*)$', line, re.I)
    if last_unit_num:
        return float(last_unit_num.group(1).replace(',', '.')) / \
            float(last_unit_num.group(2).replace(',', '.').replace(' ', '')), last_unit_num.group(3)
    last_unit_num = re.search(r'@?unit:? +10\^(-?\d+) (.*)$', line, re.I)
    if last_unit_num:
        return 10**float(last_unit_num.group(1)), last_unit_num.group(2)
    last_unit_num = re.search(r'@?unit:? (\d+|\d,\d+) *%(.*)$', line, re.I)
    mult = 0.01
    if not last_unit_num:
        last_unit_num = re.search(r'@?unit:? +(\d+ |\d+,\d+ )?(.*)$', line, re.I)
        mult = 1
    if last_unit_num.group(1) is None:
        # no unit
        return 1, last_unit_num.group(2)
    return float(last_unit_num.group(1).replace(',', '.')) * mult, last_unit_num.group(2)

if __name__ == '__main__':
    files = sys.argv[1:]
    for fn in files:
        with open(fn, 'r') as f:
            lines = f.readlines()
        def is_unit_line(line):
            return re.search(r'^\s*\*?\s*@?unit:?\s', line, re.I)
        lines = [line.strip() for line in lines if is_unit_line(line) or '::=' in line]

        rep = {
            '*': '_',
            '/': '_',
            '+': '_',
            '-': '_',
            '^': '_',
            '(': '_',
            ')': '_',
            ',': '_',
            '.': '_',
            ' ': '_',
        }

        last_unit_num = None
        last_unit = None
        print("#pragma once")
        for line in lines:
            if is_unit_line(line):
                last_unit_num, last_unit = get_last_unit(line)
                # print(line)
                # print(last_unit_num, last_unit)
                for s, r in rep.items():
                    last_unit = last_unit.strip().replace(s, r)
            if '::=' in line:
                if last_unit is None:
                    continue
                name = line.split('::=')[0].strip()
                val = f"static constexpr double {name}Unit_{last_unit} = {last_unit_num};"
                val = val.replace("__", '_').replace('__', '_')
                print(val)
                last_unit = None
