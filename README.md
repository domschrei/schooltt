# School timetabling problem generator and SAT encoder

Some of the CNF instances from this project have been submitted and used in SAT Competition 2026 (but no earlier year).

## Building

`bash build.sh`

## Generating Problems

This command produces `schooltt-5-3-8-2-4-5-10-0.1-0.1-3.cnf`, one of the SAT instances from SAT competition 2026:

```bash
bash generate-cnf.sh 5 3 8 2 4 5 10 0.1 0.1 3
```

Execute `./generator` to display help regarding the program options. Note that some combinations of parameters may be broken or result in a problem infeasible to generate and write, let alone solve. See our benchmark submission paper below for some more details.

## Solving and Checking

Use `./timetabler <school-tt-file> <cnf-output> <solver-bin-path> > out` to attempt to solve the resulting CNF problem with the provided SAT solver (skipping the encoding procedure if the CNF already exists), and then use `./timetabler <school-tt-file> <cnf-output> out` to check the solution (if the solver produced one).

## Reference

https://satres.eu/papers/2026-isc-school.pdf
