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

## SAMPIC event-builder QA

The SAMPIC profile additionally decodes the event builder's `BD00` waveform
bank and `BTnn`/`BC00`/`BS00` diagnostic banks. It preserves the existing SD/ST/SC
readers. New ROOT branches are:

- `sampic_builder_event` and `has_sampic_builder_event`: uint64 collection bounds,
  plus hits containing `crate`, `data` (the existing SD hit type), and `metadata`
  (the existing SH metadata type). Encoded waveforms are signed int16; corrected
  waveforms are volts when ADC-corrected, otherwise ADC counts. Original hardware
  timestamps remain double nanoseconds, preserving their fractional parts.
- `sampic_builder_diagnostics` and `has_sampic_builder_diagnostics`: a vector
  of per-crate acquisition summaries, and an optional collector-cycle record.
  Both `BT00` and `BT01` are retained when present in one MIDAS event. Acquisition
  durations are microseconds; builder durations are nanoseconds. Cycle IDs join
  summaries to collector cycles; they are not physics-event IDs. BS00 adds
  `has_cut_summary`, `cut_cycle_id`, and ordered `cut_summaries` with cut name,
  enabled state, signed priority and evaluated/rejected event/hit counts.
  Counters are per-cycle deltas. Lower priority runs first (ID breaks ties);
  later cuts only see survivors. Disabled cuts have zero counters.

BD uses the `SEB1` magic/version 4 (28-byte header and 188-byte hit records); BT uses `BTS1` version 1 and BC uses `BCT1`
version 1. Timing records are 128 bytes. BS uses `BSS1` version 1, a 24-byte header
and 72-byte cut records, independently of timing enablement. The diagnostics stage's `bank_names`
parameter selects input banks; the default profile enables exactly two crates.
The builder emits diagnostic-only MIDAS events, so use the `has_*` branches
before reading an object. Neither timing nor waveform presence implies the other.

Build against the local plugin sources in the existing Conda environment:

```bash
conda activate root_env
export UNPACKER_DATA_PRODUCTS_SAMPIC_SOURCE=/home/jack/cpp_projects/analysis_pipeline_project/plugins/sampic/unpacker_data_products_sampic
export UNPACKER_STAGES_SAMPIC_SOURCE=/home/jack/cpp_projects/analysis_pipeline_project/plugins/sampic/unpacker_stages_sampic
./scripts/build.sh -j 2
```

CMake needs `tbb-devel`, in addition to the TBB runtime, in that environment.

Verify a recording and produce `.artifacts/builder_qa/output.root`:

```bash
python tests/verify_builder_banks.py /absolute/path/to/run00002.mid \
  --manifest /absolute/path/to/recording/manifest.json
python tests/check_builder_edge_cases.py
python tests/execute_notebooks.py --notebook SAMPIC/event_builder_qa.ipynb \
  --builder-manifest /absolute/path/to/recording/manifest.json
```

The verification compares every ROOT waveform sample, ID, timestamp and timing
field with an independent MIDAS wire parse, then compares per-crate counts with
the recording manifest. It checks acquisition ID continuity and collector totals.
Retained hits plus BS rejected hits must match source totals. The checks cover
disabled cuts, dummy rejection, topology rejection, reordered priorities, timing
disabled and all-events-rejected recordings.
Malformed-bank tests check truncation, versions, counts, signed ADC extremes,
fractional double timestamps, full-width integer counters, and compatibility with the original ST/SC layouts.
Malformed banks are logged and omitted, consistent with existing unpackers;
verification treats any unpacker warning or error as a failure.

Open `notebooks/SAMPIC/event_builder_qa.ipynb` with the `root_env` kernel. Its
first code cell selects the ROOT file and recording manifest. It plots waveform
examples, signal distributions, channel occupancy, built-event sizes, acquisition
phase timings, builder-cycle timings, and per-cut counters. It prints bank field types, offsets,
sizes, units, observed bank sizes and decoded example records. PNG/PDF figures are saved beside the
ROOT file in `figures/`. Executed copies from the notebook runner go under
`.artifacts/notebook_verification/builder_qa/SAMPIC/`; the source remains free of
execution outputs. These low-rate simulated data verify the software formats and
accounting, rather than hardware calibration or sustained throughput.


### HDSoC event-builder banks and QA

The HDSoC profile supports optional `HB00`, `HC00`, and `HS00` alongside legacy
`HD00`/`HT00`. Builder data preserves each source board ID, nested collector
header/packets/footer, assembled ADC waveforms, and optional collector timing.
ROOT branches are `nalu_builder_event`, `nalu_collection_timing`, and
`nalu_selection_summary`, each with a `has_nalu_*` presence flag. Empty or
selection-cleared builder data remains an entry with its header and summaries.
The decoder checks versions, sizes, source framing and counters; it applies no
hardware calibration.

To build against local HDSoC plugin checkouts in the existing ROOT environment:

```bash
export UNPACKER_DATA_PRODUCTS_NALU_SOURCE=/path/to/plugins/nalu/unpacker_data_products_nalu
export UNPACKER_STAGES_NALU_SOURCE=/path/to/plugins/nalu/unpacker_stages_nalu
./scripts/build.sh
```

Independently compare a simulated MIDAS recording with the unpacked ROOT fields,
packets and waveforms (outputs stay under `.artifacts`):

```bash
python tests/verify_hdsoc_builder_banks.py /path/to/run.mid.lz4 \
  --output .artifacts/hdsoc_builder_qa/no_cuts --simulated
```

Open `notebooks/HDSoC/event_builder_qa.ipynb` in the Python kernel from `root_env`.
Its data cell selects `DATASET = 'no_cuts'` or `'dummy_cut'`; the local recordings
contain two simulated boards with 61 windows / 1,952 samples per waveform.
The notebook prints the bank field types/offsets and observed sizes, verifies
sample and selection accounting, and plots waveform boundaries, occupancy,
amplitudes and collection timings. Figures go beside the ROOT file. Set
`CHECK_SIMULATOR = False` for hardware data.

Decoder boundary/malformed-bank tests can be enabled with
`cmake -S . -B build -DNALU_BUILDER_TESTS=ON` and run with
`ctest --test-dir build/_deps/unpacker_stages_nalu-build --output-on-failure`.
