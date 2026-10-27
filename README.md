# Clinic Waiting-Line Simulator

A C++ discrete-event simulation of patients arriving at a clinic and waiting for staff. It answers one question: how many staff keep the average wait under a target?

## Model

- Patients arrive at random (Poisson arrivals).
- Each patient needs a random amount of staff time (exponential service times).
- One shared line, first come first served, `c` identical staff (an M/M/c queue).

## Build and run

```
g++ -O2 -std=c++17 clinic_sim.cpp -o clinic_sim
./clinic_sim 10 12 3 8 results.csv
```

Arguments: arrivals per hour, minutes per patient, fewest staff, most staff, output file.

For each staff count the program runs 30 replications of 2,000 clinic hours (100 warm-up hours discarded, fixed random seeds) and writes the average wait with a 95% confidence interval, average number waiting, staff utilization, and the chance a patient waits.

## Validation

`clinic_analysis.xlsx` compares the simulation to the Erlang C formula and checks Little's Law (Lq = λ × Wq) using only simulation output.

With 10 patients per hour and 12 minutes per patient:

| Staff | Simulated wait (min) | Erlang C wait (min) | Utilization |
|---|---|---|---|
| 3 | 5.28 | 5.33 | 66.7% |
| 4 | 1.04 | 1.04 | 49.9% |
| 5 | 0.24 | 0.24 | 40.0% |

The simulation falls inside its 95% interval around the theory at every staffing level from 3 to 8. Four staff meet a 2-minute average wait target.

## Assumptions

The arrival rate, service time and wait target are assumed values for a small clinic, not measured data.

## Files

- `clinic_sim.cpp`: the simulator
- `results.csv`: output for 3 to 8 staff
- `clinic_analysis.xlsx`: Erlang C comparison, Little's Law check, and chart
