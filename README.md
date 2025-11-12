# MIDAS File Unpacker App

This application unpacks MIDAS `.mid` or `.mid.lz4` files into ROOT TTrees using the [analysis_pipeline](https://github.com/jaca230/analysis_pipeline) framework and associated unpacker plugins.

All dependencies are pulled automatically via [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake). No manual library installs are required, except for ROOT and a working C++17 toolchain.

---

## Requirements

- **CMake** ≥ 3.18
- **C++17** compiler (GCC, Clang, or MSVC)
- **ROOT** (must be installed and discoverable via `find_package(ROOT ...)`)
- **Git** (required for CPM to fetch dependencies)
- Optional: **Valgrind** or **gdb** (for debugging and memory analysis)

---

## Building

1. Clone the repository:

   ```bash
   git clone https://github.com/jaca230/midas_file_unpacker_app
   cd midas_file_unpacker_app
   ```

2. Run the build script:

   ```bash
   ./scripts/build.sh
   ```

   By default, this will:

   * Download and build all dependencies via CPM
   * Build the `unpacker` executable in `build/bin/`
   * Build plugin libraries in `build/lib/`

   Options:

   * `-o` / `--overwrite`: clean build directory before rebuilding
   * `-j <N>`: specify number of build jobs (default: all available cores)

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

CPM lets you point any dependency at a local checkout by setting `CPM_<package>_SOURCE`
before configuring, e.g.

```bash
cmake -DCPM_unpacker_stages_sampic_SOURCE=/path/to/local/unpacker_stages_sampic ..
```

This replaces the remote GitHub clone for that package, which is useful when developing
multiple repos in tandem.

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
* Access unpacked SAMPIC (`SampicEvent`, `SampicEventTiming`, `SampicCollectorTiming`) or
  HDSoC (`NaluEvent`, `NaluTime`) data products and waveform information
* Perform quick checks and validation on the output

For a starting point, see:

* `notebooks/example.ipynb` — basic usage demonstration

---

## Notes

* CPM will clone external repositories under `build/_deps/`.
* ROOT must be installed separately and discoverable by CMake (`source thisroot.sh`).
* MIDAS file I/O is bundled with the `midas_event_unpacker_plugin` (no external MIDAS dependency required).

---

## License

[MIT License](LICENSE)
