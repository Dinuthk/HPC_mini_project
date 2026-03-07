#include "compute.h"
#include <omp.h>

void compute_risk_batch(Trade* batch_trades, double* batch_results, int num_trades) {
    #pragma omp parallel for
    for (int idx = 0; idx < num_trades; idx++) {
        double risk = 0.0;
        for (int i = 0; i < 1000; i++) {
            risk += (batch_trades[idx].price * 0.01) + (batch_trades[idx].volume * 0.0001) - (i * 0.001); 
        }
        batch_results[idx] = risk;
    }
}