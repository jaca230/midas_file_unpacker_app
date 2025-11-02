# SAMPIC Waveform Analysis Notebooks

This directory contains Jupyter notebooks for analyzing SAMPIC data unpacked from MIDAS files.

## Notebooks

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

## Getting Started

### Prerequisites

Install required Python packages:
```bash
pip install numpy matplotlib jupyter
```

**Note**: These notebooks use PyROOT (ROOT's Python bindings) which is included with ROOT. Make sure you have ROOT installed and properly configured in your Python environment.

### Running the Notebooks

1. **Generate output.root file** (if you haven't already):
   ```bash
   cd /home/jack/cpp_projects/analysis_pipeline_project/apps/midas_file_unpacker_app
   ./build/bin/unpacker /home/jack/misc/midas_stuff/sampic_sim_daq_data/run00122.mid.lz4 10000
   ```

2. **Start Jupyter**:
   ```bash
   jupyter notebook
   ```

3. **Open a notebook** and run the cells!

## Quick Example

```python
import ROOT
import numpy as np
import matplotlib.pyplot as plt
import os

# Load libraries
BUILD_LIB_PATH = "../build/lib"
for lib in ["libanalysis_pipeline_core.so", "libunpacker_data_products_core.so",
            "libunpacker_data_products_sampic.so"]:
    ROOT.gSystem.Load(os.path.join(BUILD_LIB_PATH, lib))

# Open file
f = ROOT.TFile.Open("../output.root")
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

The ROOT file contains a TTree called `events` with branches:
- `sampic_event.hits.channel` - Channel number (0-63)
- `sampic_event.hits.corrected_waveform` - Vector of float samples
- `sampic_event.hits.amplitude` - Hit amplitude
- `sampic_event.hits.baseline` - Waveform baseline
- `sampic_event.hits.peak` - Waveform peak value
- `sampic_event.hits.time_instant` - Hit time (ns)
- `sampic_event.hits.tot_value` - Time over threshold (ns)
- `sampic_event.hits.fe_board_index` - Frontend board index
- `sampic_event.hits.sampic_index` - SAMPIC chip index
- And more...

## Notes

- Notebooks expect `output.root` to be in the parent directory (`../output.root`)
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
