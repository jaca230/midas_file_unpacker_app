# MIDAS File Unpacker App

This application unpacks MIDAS `.mid` or `.mid.lz4` files into ROOT TTrees using the [analysis_pipeline](https://github.com/jaca230/analysis_pipeline) framework and associated unpacker plugins.

All dependencies are pulled automatically via [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake). No manual library installs are required, except for ROOT and a working C++17 toolchain.

---

## Development environment

The repository provides a project-local Micromamba environment containing
ROOT, TBB, the compiler toolchain, Jupyter, and the Python analysis packages.
It does not require a system Conda installation:

```bash
source scripts/setup_env.sh
```

The first activation creates `.venv/` automatically. To deactivate it:

```bash
source scripts/deactivate_env.sh
```

Git is also required because CPM fetches the pipeline libraries and plugins.

---

## Building

1. Clone the repository:

   ```bash
   git clone https://github.com/jaca230/midas_file_unpacker_app
   cd midas_file_unpacker_app
   ```

2. Run the build script:

   ```bash
   source scripts/setup_env.sh
   ./scripts/build.sh
   ```

   By default, this will:

   * Download and build all dependencies via CPM
   * Build the `unpacker` executable in `build/bin/`
   * Build plugin libraries in `build/lib/`

   Options:

   * `-o` / `--overwrite`: clean build directory before rebuilding
   * `-j <N>`: specify the number of build jobs (default: 2)

   The default can also be set with the generic
   `UNPACKER_BUILD_JOBS` environment variable.

   CPM keeps `FetchContent` connected to the network, so each configure step fetches the
   latest commits for dependencies that follow a branch (e.g., `main`).

---

## Running

To run the unpacker on a MIDAS file, use the `run.sh` helper script. By default the
application unpacks **SAMPIC** data products; pass `--profile HDSoC` to switch to the
HDSoC/Nalu pipeline.

```bash
# Default SAMPIC pipeline (all events)
./scripts/run.sh -- path/to/input.mid.lz4

# SAMPIC pipeline with an explicit event limit
./scripts/run.sh -- path/to/input.mid.lz4 --max-events 1000

# HDSoC / Nalu pipeline
./scripts/run.sh --profile HDSoC -- path/to/input.mid.lz4

# Show available options
./build/bin/unpacker --help
```

The output is written to `output.root` and contains a TTree named `events` with either
SAMPIC or HDSoC data products depending on the selected profile.

### Run-time Options

* `-d` / `--debug`: Run under `gdb`
* `-v` / `--valgrind`: Run under `valgrind`
* `--preload <libs>`: LD\_PRELOAD extra shared libraries (comma-separated)
* `--profile <name>`: Default pipeline profile (`SAMPIC` or `HDSoC`). You can also pass
  `--profile` after the `--` separator and it will be forwarded directly to the executable.
* `--max-events <N>`: Limit the number of events processed. (You can also pass a numeric
  positional argument for backwards compatibility.)

---

## Configuration

The application loads configuration files at runtime:

* **Logger config**: `config/logger.json`
* **Pipeline configs**:
  * `config/unpacker_pipelines/SAMPIC/default_unpacking_pipeline.json`
  * `config/unpacker_pipelines/HDSoC/default_unpacking_pipeline.json`

The executable chooses which pipeline to load at runtime based on `--profile`. Adjust these
JSON files to change logging, pipeline stages, or plugin paths.

### Overriding dependencies for local development

For coordinated SAMPIC plugin development, copy `.env.example` to `.env` and
set either local checkout:

```dotenv
UNPACKER_DATA_PRODUCTS_SAMPIC_SOURCE=/path/to/unpacker_data_products_sampic
UNPACKER_STAGES_SAMPIC_SOURCE=/path/to/unpacker_stages_sampic
```

The build script forwards these paths to CPM. When unset, the application
fetches both plugins from GitHub and remains completely standalone.

---

## Cleaning

To remove build artifacts:

```bash
./scripts/cleanup.sh
```

This will delete `build/`, `bin/`, and `lib/`.

---

## Project Structure

```
apps/midas_file_unpacker_app/
├── CMakeLists.txt
├── config/                # JSON config files
├── scripts/               # Build/run/cleanup scripts
├── src/                   # Application sources
├── notebooks/             # Example analysis notebooks
└── output.root            # Example output file
```

---

## Examples

Several Jupyter notebooks are provided in the `notebooks/` directory to demonstrate
how to:

* Open and inspect the produced `output.root` file
* Access unpacked SAMPIC (`SampicEvent`, `SampicHitMetadata`, `SampicEventTiming`,
  `SampicCollectorTiming`, `SampicTriggerGroup`, `SampicTriggerRecords`) or HDSoC
  (`NaluEvent`, `NaluTime`) data products and waveform information
* Perform quick checks and validation on the output

For a starting point, see:

* `notebooks/SAMPIC/quick_waveform_plot.ipynb` — sparse-event-safe waveform overview
* `notebooks/SAMPIC/timing_analysis.ipynb` — SAMPIC and collector timing
* `notebooks/SAMPIC/external_trigger_diagnostics.ipynb` — TG bank continuity,
  cadence, hit assignment, and timestamp residuals

---

## Streaming HDSoC heatmaps

Generate the sample-index versus ADC heatmaps from
`notebooks/HDSoC/windows_check.ipynb` without collecting all samples in memory:

```bash
python notebooks/playground/plot_hdsoc_heatmap.py output.root
python notebooks/playground/plot_hdsoc_heatmap.py data/run*.root --channels 0 2 4 --output output/heatmap.png
```

Channels default to `0 2`; the output defaults to `hdsoc_heatmap.png`.
Run in the development environment with ROOT, NumPy, and Matplotlib installed,
using the application's built libraries in `build/lib` (override with
`--lib-dir`). The script works without a display.

Two streaming passes find per-channel ranges and accumulate 200×200 histograms,
matching the notebook's automatic bin ranges and 32-sample window markers.
Memory holds fixed-size histograms plus the current ROOT event, read buffers,
and temporary arrays for one waveform; it does not grow with the total number
of events. Split ROOT trees read only waveform branches. Multiple input files
are combined into one heatmap per selected channel and must remain unchanged
between passes. Use `--bins 200 200` to change resolution or `--max-events 1000`
for a quick check (the limit is across all files, applied on both passes).

---

## Notes

* CPM will clone external repositories under `build/_deps/`.
* ROOT and TBB are supplied by the local development environment.
* MIDAS file I/O is bundled with the `midas_event_unpacker_plugin` (no external MIDAS dependency required).

---

## License

[MIT License](LICENSE)


## Current DAQ bank formats (November 2026)

The SAMPIC profile expects compact output (`verbose_banks=false`): SD `SDC1`
version 1 (154 bytes/hit), SH version 2 (32 bytes/hit), and the existing ST/SC/SG/SV
banks. It also decodes SR periodic-readout and SB fixed-time-bin banks, for frontend
suffixes `00` and `01`. ROOT adds `sampic_readout_tick`, `sampic_time_bin`, and their
`has_*` presence flags; metadata-only and zero-hit events are retained.

`encoded_waveform` stores the valid signed 16-bit wire samples. `corrected_waveform`
converts them to volts when ADC correction is enabled (divide by 10000), or ADC
counts otherwise. Correction and clipping/nonfinite flags are preserved. Fields
removed from compact SD/SH are no longer exposed as misleading zero values.
Historical SD and SH formats are not decoded.

The HDSoC `HD`/`HT` layouts remain 36-byte event header + 80-byte packets + 2-byte
footer, and 64-byte timing data. Both trigger-time and periodic/time-batch modes
use the same layout. Waveforms are grouped by `(channel, trigger_time)` within
each MIDAS event, including batches containing multiple triggers.

Build with the matching local SAMPIC data-product and stage checkouts using the
`UNPACKER_DATA_PRODUCTS_SAMPIC_SOURCE` and `UNPACKER_STAGES_SAMPIC_SOURCE` overrides
above. Then, in an environment with PyROOT and the `lz4` executable:

```bash
python tests/verify_current_banks.py
python tests/check_bank_edge_cases.py
```

The first command unpacks and compares every event, hit, packet, waveform sample,
and timing field in runs 121479, 121485, 00266, and 00267. The second checks
synthetic boundary cases and rejection of malformed banks. Outputs and logs go
to `.artifacts/bank_format_verification/`, with one ROOT file per real run.
The original input files and the app's usual `output.root` are not overwritten.


Notebook setup, current-schema units, and the execution matrix are documented in
[notebooks/README.md](notebooks/README.md). Run `python tests/execute_notebooks.py`
to execute all maintained notebooks on both verified acquisition modes.
