#include "compute.h"
#include <omp.h>
#include <math.h> // For computationally heavy mathematical functions

void compute_risk_batch(Trade* batch_trades, double* batch_results, int num_trades) {
    // Executes in parallel across all available CPU cores using OpenMP
    #pragma omp parallel for
    for (int idx = 0; idx < num_trades; idx++) {
        
        double base_price = batch_trades[idx].price;
        double volume = batch_trades[idx].volume;
        double weight = batch_trades[idx].complexity_weight;
        
        double total_simulated_risk = 0.0;
        
        // Iteration count increased to 50,000 to place a heavy load on the system
        int simulations = 50000; 
        
        for (int i = 0; i < simulations; i++) {
            // Generating a pseudo-random noise value
            double pseudo_rand = (double)(i % 100) / 100.0;
            
            // Complex mathematical equation mimicking Black-Scholes / Monte Carlo models 
            // used in real-world finance to demand heavy CPU capacity:
            double drift = (base_price * 0.02) - (volume * 0.00005);
            double volatility = sin(base_price * pseudo_rand) * cos(volume * 0.001) * weight;
            
            // exp() and log() functions impose a significant computational burden on the CPU
            double simulated_price = base_price * exp(drift + volatility + log(1.0 + pseudo_rand));
            
            total_simulated_risk += simulated_price;
        }
        
        // Storing the final average simulated risk value into the results buffer
        batch_results[idx] = total_simulated_risk / simulations;
    }
}