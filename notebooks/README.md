# Waveform Analysis Notebooks

This directory now groups notebooks by hardware profile:

- `SAMPIC/` – tools for SAMPIC waveform and timing analysis
- `HDSoC/` – notebooks for the legacy HDSoC / Nalu profile (`--profile HDSoC`)

## SAMPIC notebooks (`SAMPIC/`)

### 1. `quick_waveform_plot.ipynb`
**Quick interactive waveform viewer** - Best for browsing individual waveforms

Features:
- Simple `plot_hit(event_num, hit_num)` function for fast exploration
- Full event visualization (all hits at once)
- Search for high-amplitude hits
- Basic statistics and distributions
- Minimal dependencies, easy to use

**Start here if you want to quickly look at waveforms!**

### 2. `plot_sampic_waveforms.ipynb`
**Comprehensive analysis notebook** - Best for detailed studies

Features:
- Individual and multi-hit waveform plots
- Statistical analysis across many events
- Distribution plots (amplitude, baseline, channel occupancy, TOT)
- 2D correlation plots (peak vs amplitude)
- Per-channel statistics
- Average waveform shape calculation

### 3. `timing_analysis.ipynb`
**Timing and performance analysis** - Diagnostic data from SAMPIC DAQ

Features:
- Event timing distributions (prepare, read, decode phases)
- Collector timing analysis (wait, group, finalize phases)
- Timing breakdown visualizations
- Performance vs event size correlations
- Summary statistics for all timing metrics
- Analysis of optional collector timing data

## HDSoC / Nalu notebooks (`HDSoC/`)

### 1. `example.ipynb`
**Minimal HDSoC waveform exploration** - Mirrors the original branch example

Features:
- Loads `nalu_event` and `nalu_time` branches produced with `--profile HDSoC`
- Basic sanity checks for library loading and branch availability
- Reference helper functions for ROOT-native exploration

### 2. `windows_check.ipynb`
**Window accounting diagnostics** - Inspect packet and waveform windows

Features:
- Packet counting per channel and event
- Waveform accumulation helpers for QA plots
- Lightweight approach for verifying unpacked HDSoC data

### 3. `plot_nalu_waveforms.ipynb`
**HDSoC / Nalu waveform viewer** - Updated HDSoC notebook

Features:
- Quick summary of event headers and timing information
- Helpers to plot individual waveforms or small collections
- Mirrors the functionality of the original HDSoC-only branch notebooks

## Getting Started

### Prerequisites

Install required Python packages:
```bash
pip install numpy matplotlib jupyter
```

**Note**: These notebooks use PyROOT (ROOT's Python bindings) which is included with ROOT. Make sure you have ROOT installed and properly configured in your Python environment.

### Running the Notebooks

1. **Generate `output.root` file** (if you haven't already):
   ```bash
   cd /home/jack/cpp_projects/analysis_pipeline_project/apps/midas_file_unpacker_app
   ./build/bin/unpacker /home/jack/misc/midas_stuff/sampic_sim_daq_data/run00122.mid.lz4 10000
   ```

2. **Start Jupyter**:
   ```bash
   jupyter notebook
   ```

3. **Open a notebook** and run the cells! Remember to pick the notebook that matches the pipeline profile you used (`SAMPIC` vs `HDSoC`).

## Quick Example

```python
import ROOT
import numpy as np
import matplotlib.pyplot as plt
import os

# Load libraries
BUILD_LIB_PATH = "../../build/lib"
for lib in ["libanalysis_pipeline_core.so", "libunpacker_data_products_core.so",
            "libunpacker_data_products_sampic.so"]:
    ROOT.gSystem.Load(os.path.join(BUILD_LIB_PATH, lib))

# Open file (path is relative to notebook directory)
f = ROOT.TFile.Open("../../output.root")
tree = f.Get("events")

# Get first event
tree.GetEntry(0)
event = tree.sampic_event
hit = event.hits[0]

# Convert waveform to numpy
waveform = np.array([hit.corrected_waveform[i] for i in range(len(hit.corrected_waveform))],
                    dtype=np.float32)

# Plot
plt.plot(waveform)
plt.xlabel('Sample Index')
plt.ylabel('Amplitude [ADC]')
plt.title(f'Channel {hit.channel}, Amplitude {hit.amplitude:.2f}')
plt.show()
```

## Data Structure

The ROOT file contains a TTree called `events` with three main branches:

### `sampic_event` - Physics Data (always present)
- `hits.channel` - Channel number (0-63)
- `hits.corrected_waveform` - Vector of float samples
- `hits.amplitude` - Hit amplitude
- `hits.baseline` - Waveform baseline
- `hits.peak` - Waveform peak value
- `hits.time_instant` - Hit time (ns)
- `hits.tot_value` - Time over threshold (ns)
- `hits.fe_board_index` - Frontend board index
- `hits.sampic_index` - SAMPIC chip index
- And more...

### `sampic_event_timing` - Event Timing Data (always present)
- `fe_timestamp_ns` - Frontend event timestamp (ns)
- `nhits` - Number of hits in event
- `nparents` - Number of parent SAMPIC events
- `sp_prepare_us_sum` - Sum of SAMPIC prepare times (us)
- `sp_read_us_sum` - Sum of SAMPIC read times (us)
- `sp_decode_us_sum` - Sum of SAMPIC decode times (us)
- `sp_total_us_sum` - Sum of SAMPIC total times (us)
- `sp_prepare_us_max` - Max SAMPIC prepare time (us)
- `sp_read_us_max` - Max SAMPIC read time (us)
- `sp_decode_us_max` - Max SAMPIC decode time (us)
- `sp_total_us_max` - Max SAMPIC total time (us)

### `sampic_collector_timing` - Collector Timing Data (optional)
- `collector_timestamp_ns` - Collection cycle start timestamp (ns)
- `n_events` - Number of frontend events in cycle
- `total_hits` - Total hits across all events
- `wait_us` - Wait duration (us)
- `group_build_us` - Grouping phase duration (us)
- `finalize_us` - Finalization duration (us)
- `total_us` - Total collection time (us)

### `has_sampic_collector_timing` - Presence Flag
- Boolean branch set to `True` when the AC00 bank was present for the event. Use this flag before dereferencing `sampic_collector_timing`.

**Note**: The `sampic_collector_timing` branch may not contain valid physics data for every event; rely on `has_sampic_collector_timing` to test presence.

## Notes

- Notebooks expect `output.root` to be two directories up (`../../output.root`)
- Uses PyROOT for reading ROOT files (requires ROOT installation)
- Libraries must be loaded before accessing custom data products
- Waveforms are stored as variable-length vectors (length = data_size field)
- One event can contain multiple hits from different channels

## Troubleshooting

**Problem**: `FileNotFoundError: ../output.root`
- **Solution**: Run the unpacker first to generate the ROOT file

**Problem**: `AttributeError` when accessing data
- **Solution**: Make sure to load all required libraries before opening the ROOT file

**Problem**: `ModuleNotFoundError: No module named 'ROOT'`
- **Solution**: Make sure ROOT is properly installed and configured in your Python environment. Try `import ROOT` in a Python shell.

**Problem**: Plots don't show up
- **Solution**: Add `%matplotlib inline` at the top of the notebook

## Performance

The unpacker processes ~38,500 events/second with 99.999% success rate when tested on the SAMPIC simulation data.
