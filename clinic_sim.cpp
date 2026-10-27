// Clinic Waiting-Line Simulator
// Simulates patients arriving at a clinic with c identical staff (M/M/c, first come first served).
// Arrivals are Poisson (exponential gaps); service times are exponential.
// For each staff count it runs several independent replications and reports averages
// with a 95% confidence interval. Output is a CSV that the Excel workbook reads.
//
// Build:  g++ -O2 -std=c++17 clinic_sim.cpp -o clinic_sim
// Run:    ./clinic_sim [arrivals_per_hour] [minutes_per_patient] [min_staff] [max_staff] [output.csv]
// Example: ./clinic_sim 10 12 3 8 results.csv

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <vector>

struct RepResult {
    double avgWaitMin;       // mean time in queue, minutes
    double avgQueueLen;      // mean number waiting, seen by arriving patients
    double utilization;      // fraction of staff time spent serving
    double probWait;         // fraction of patients who had to wait
    double probWaitOver15;   // fraction who waited more than 15 minutes
};

// One replication: simulate `hours` of clinic time, ignore the first `warmup` hours.
RepResult runOnce(double lambdaPerHr, double meanServiceMin, int staff,
                  double hours, double warmup, std::mt19937_64& rng) {
    std::exponential_distribution<double> gap(lambdaPerHr / 60.0);        // per minute
    std::exponential_distribution<double> service(1.0 / meanServiceMin);  // per minute

    const double endTime = hours * 60.0;
    const double warmTime = warmup * 60.0;

    // Min-heap of the times each staff member becomes free.
    std::priority_queue<double, std::vector<double>, std::greater<double>> freeAt;
    for (int i = 0; i < staff; ++i) freeAt.push(0.0);

    // Min-heap of start times for patients currently waiting (to count the queue).
    std::priority_queue<double, std::vector<double>, std::greater<double>> waitingStarts;

    double t = 0.0;
    long long counted = 0, waited = 0, waitedOver15 = 0;
    double sumWait = 0.0, sumQueueSeen = 0.0, busyTime = 0.0;

    while (true) {
        t += gap(rng);
        if (t >= endTime) break;

        // Remove patients who have already started service.
        while (!waitingStarts.empty() && waitingStarts.top() <= t) waitingStarts.pop();

        double earliestFree = freeAt.top();
        freeAt.pop();
        double start = std::max(t, earliestFree);
        double s = service(rng);
        freeAt.push(start + s);

        double wait = start - t;
        if (wait > 0.0) waitingStarts.push(start);

        if (t >= warmTime) {
            ++counted;
            sumWait += wait;
            sumQueueSeen += static_cast<double>(waitingStarts.size()) - (wait > 0.0 ? 1.0 : 0.0);
            if (wait > 0.0) ++waited;
            if (wait > 15.0) ++waitedOver15;
            busyTime += s;
        }
    }

    RepResult r;
    r.avgWaitMin = sumWait / counted;
    r.avgQueueLen = sumQueueSeen / counted;
    r.utilization = busyTime / ((endTime - warmTime) * staff);
    r.probWait = static_cast<double>(waited) / counted;
    r.probWaitOver15 = static_cast<double>(waitedOver15) / counted;
    return r;
}

// Mean and 95% confidence half-width across replications (t value for 29 df).
void meanAndCI(const std::vector<double>& x, double& mean, double& half) {
    const double n = static_cast<double>(x.size());
    mean = 0.0;
    for (double v : x) mean += v;
    mean /= n;
    double ss = 0.0;
    for (double v : x) ss += (v - mean) * (v - mean);
    double sd = std::sqrt(ss / (n - 1.0));
    const double tCrit = 2.045;  // 95%, 29 degrees of freedom (30 replications)
    half = tCrit * sd / std::sqrt(n);
}

int main(int argc, char** argv) {
    double lambdaPerHr = argc > 1 ? std::stod(argv[1]) : 10.0;
    double meanServiceMin = argc > 2 ? std::stod(argv[2]) : 12.0;
    int minStaff = argc > 3 ? std::stoi(argv[3]) : 3;
    int maxStaff = argc > 4 ? std::stoi(argv[4]) : 8;
    std::string outFile = argc > 5 ? argv[5] : "results.csv";

    const int replications = 30;
    const double hoursPerRep = 2000.0;
    const double warmupHours = 100.0;

    std::cout << "Arrivals per hour: " << lambdaPerHr << "\n"
              << "Minutes per patient: " << meanServiceMin << "\n"
              << "Offered load (staff needed just to keep up): "
              << lambdaPerHr * meanServiceMin / 60.0 << "\n\n";

    std::ofstream out(outFile);
    out << "staff,avg_wait_min,wait_ci95,avg_queue_len,utilization,prob_wait,prob_wait_over_15\n";

    for (int c = minStaff; c <= maxStaff; ++c) {
        double load = lambdaPerHr * meanServiceMin / 60.0;
        if (load >= c) {
            std::cout << "Staff " << c << ": skipped, arrivals exceed capacity\n";
            continue;
        }
        std::vector<double> w, q, u, pw, p15;
        for (int rep = 0; rep < replications; ++rep) {
            std::mt19937_64 rng(1000ULL * c + rep + 1);  // fixed seeds, so results repeat
            RepResult r = runOnce(lambdaPerHr, meanServiceMin, c, hoursPerRep, warmupHours, rng);
            w.push_back(r.avgWaitMin);
            q.push_back(r.avgQueueLen);
            u.push_back(r.utilization);
            pw.push_back(r.probWait);
            p15.push_back(r.probWaitOver15);
        }
        double mw, hw, mq, hq, mu, hu, mpw, hpw, mp15, hp15;
        meanAndCI(w, mw, hw);
        meanAndCI(q, mq, hq);
        meanAndCI(u, mu, hu);
        meanAndCI(pw, mpw, hpw);
        meanAndCI(p15, mp15, hp15);

        out << c << "," << mw << "," << hw << "," << mq << "," << mu << ","
            << mpw << "," << mp15 << "\n";
        std::cout << "Staff " << c << ": wait " << mw << " min (+/- " << hw << "), queue "
                  << mq << ", utilization " << mu * 100.0 << "%\n";
    }
    std::cout << "\nWrote " << outFile << "\n";
    return 0;
}
