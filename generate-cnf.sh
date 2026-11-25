#!/bin/bash

set -e

./generator $@ > .out
./timetabler .out schooltt-$(echo "$@"|tr ' ' '-').cnf
