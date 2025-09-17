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
   git clone https://github.com/your/repo.git
   cd repo/apps/midas_file_unpacker_app
````

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

---

## Running

To run the unpacker on a MIDAS file:

```bash
./scripts/run.sh -- path/to/input.mid.lz4 [max_events]
```

Examples:

```bash
# Process all events
./scripts/run.sh -- ./run42115.mid.lz4

# Process only 1000 events
./scripts/run.sh -- ./run42115.mid.lz4 1000
```

The output will be written to:

```
output.root
```

containing a TTree named `events` with unpacked Nalu data products.

### Run-time Options

* `-d` / `--debug`: Run under `gdb`
* `-v` / `--valgrind`: Run under `valgrind`
* `--preload <libs>`: LD\_PRELOAD extra shared libraries (comma-separated)

---

## Configuration

The application loads configuration files at runtime:

* **Logger config**: `config/logger.json`
* **Pipeline config**: `config/unpacker_pipelines/HDSoC/default_unpacking_pipeline.json`

Modify these JSON files to adjust logging, pipeline stages, or plugin paths.

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

## Notes

* CPM will clone external repositories under `build/_deps/`.
* ROOT must be installed separately and discoverable by CMake (`source thisroot.sh`).
* MIDAS file I/O is bundled with the `midas_event_unpacker_plugin` (no external MIDAS dependency required).

---

## License

[MIT License](LICENSE)

```