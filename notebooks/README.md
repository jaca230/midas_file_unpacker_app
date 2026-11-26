# Current-schema analysis notebooks

Use the app's ROOT/Jupyter Python environment and build the updated local SAMPIC
plugins first. The notebooks find the app from the repository root or a notebook
subdirectory, load the dictionaries, and validate the selected ROOT tree.

Select the ROOT file directly in the **first code cell** of each notebook:

```python
DATA_FILE = "/absolute/path/to/run.root"
```

Then run the notebook from the top. `~` is supported; relative paths are resolved
from the app directory, regardless of where Jupyter starts. No environment
variable is needed. The default path selects the verified periodic fixture for
the notebook's detector, and the resolved path is printed when opened.

The noise-filtering notebook is in
[`playground/noise_filtering_waveforms.ipynb`](playground/noise_filtering_waveforms.ipynb).

## Interpretation

- Compact SAMPIC has no vendor amplitude, baseline, peak, or derived hit-time
  scalars. Waveform notebooks estimate baseline from the first eight valid
  samples, amplitude as max−min, and peak as the waveform maximum. These estimates
  are not interchangeable with the removed vendor results.
- ADC-corrected samples are stored in volts and displayed in **mV**. Uncalibrated
  samples stay in ADC counts. Aggregation refuses to mix these two unit systems
  in one file; split mixed files by correction state before aggregate analysis.
- Noise-filter thresholds use display units and are illustrative, not calibrated
  selection criteria. Quality flags are retained in the ROOT hit objects.
- Empty SD banks and metadata-only entries are distinct from waveform-bearing
  entries. Optional SG/SR/SB/SC analyses report absence rather than interpreting
  stale ROOT objects as current data.
- The timing notebook estimates uncompressed MIDAS volume from known bank
  layouts and padding, not ROOT `GetEntry()` bytes. Its frontend-cadence rates are
  not measurements of compressed disk/network throughput or self-trigger rate.
  SR host time and SB/hit hardware time are separate clock domains.
- HDSoC periodic batches contain many triggers per channel. Packet diagnostics
  match both channel **and trigger timestamp**, never channel alone or a slice
  sorted by parser index. Trigger-time consistency verifies this grouping
  invariant; it cannot establish independent trigger jitter.
- HDSoC ADC samples are unsigned; timing-cycle histograms in `example.ipynb`
  deduplicate repeated HT records by collection-cycle index and timestamp.

## Validation

```bash
python tests/execute_notebooks.py
python -m unittest discover -s tests -p test_notebook_support.py
python tests/make_notebook_fixture.py
python tests/execute_notebooks.py --notebook SAMPIC/external_trigger_diagnostics.ipynb --run notebook_fixture
python tests/execute_notebooks.py --notebook SAMPIC/timing_analysis.ipynb --run notebook_fixture
```

The execution matrix covers all 11 maintained notebooks against both real modes
(22 executions), scanning complete files. SG/SV/SB branch coverage uses a clearly
identified synthetic fixture because these banks are absent from the four real
files. Executed notebooks, figures, and results go under
`.artifacts/notebook_verification/`; source notebooks have cleared outputs.
Jupyter's hidden `.ipynb_checkpoints` are autosaves, excluded from maintenance and
the test matrix.

## Event-builder recording

`SAMPIC/event_builder_qa.ipynb` uses the separate BD/BT/BC/BS ROOT branches. It runs
on `.artifacts/builder_qa/output.root` and the recording manifest selected in its
first code cell, rather than the legacy frontend fixtures above. It reads BD v4 corrected SD/SH records, plots ADC-corrected waveforms in mV,
checks double timestamp precision, and separates source microseconds from builder
nanoseconds. A bank-structure section prints field types, offsets, sizes, units
and example records.

```bash
conda activate root_env
python tests/verify_builder_banks.py /path/to/recording/data/run00002.mid --manifest /path/to/recording/manifest.json
python tests/execute_notebooks.py --notebook SAMPIC/event_builder_qa.ipynb --builder-manifest /path/to/recording/manifest.json
```

Use `--builder-root` to select a different ROOT output in the notebook runner.
The default execution matrix runs this notebook once on builder QA data, while
continuing to run the other notebooks against their frontend datasets. The seven
figures are also exported as PNG and PDF under the ROOT output's `figures/`.

BS00 QA checks priority order, survivor-only evaluation, and retained/rejected
hit accounting. Cut tests include disabled, dummy, topology, reordered priorities,
timing-disabled and all-rejected cases. `--builder-label` keeps executed notebook
copies for different configurations in separate artifact directories.


`HDSoC/event_builder_qa.ipynb` checks the two-board `HB00`/`HC00`/`HS00`
products. Select `no_cuts` or `dummy_cut` in its tagged data cell. Local datasets
are in `.artifacts/hdsoc_builder_qa/`; the notebook prints wire layouts and
checks continuous 61-window simulated waveforms, board/source identities and
cut counters. It saves PNG/PDF figures beside the selected ROOT file.
