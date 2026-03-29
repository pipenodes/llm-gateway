#ifndef BENCHMARK_RESOURCE_MONITOR_H
#define BENCHMARK_RESOURCE_MONITOR_H

namespace benchmark {

// Placeholder: RAM/VRAM/CPU/GPU/energy would be read from host (e.g. nvidia-smi, /proc).
// Ollama does not expose per-request resource usage via the API.
struct ResourceSnapshot {
    double ram_mb = 0.0;      // 0 = not measured
    double vram_mb = 0.0;
    double cpu_pct = 0.0;
    double gpu_pct = 0.0;
    double energy_wh = 0.0;   // optional
};

} // namespace benchmark

#endif
