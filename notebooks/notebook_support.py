"""Shared setup and current-schema helpers for the analysis notebooks."""
from pathlib import Path
import numpy as np
import ROOT

APP_ROOT = Path(__file__).resolve().parents[1]
_LOADED_LIBRARIES = set()


def open_data(profile, data_file):
    """Load dictionaries before opening a current-schema ROOT tree."""
    library = 'sampic' if profile == 'SAMPIC' else 'nalu'
    for name in ('analysis_pipeline_core', 'unpacker_data_products_core',
                 'unpacker_data_products_' + library):
        path = APP_ROOT / 'build/lib' / ('lib' + name + '.so')
        if name in _LOADED_LIBRARIES:
            continue
        if ROOT.gSystem.Load(str(path)) < 0:
            raise RuntimeError(f'Cannot load {path}; build the app in the active ROOT environment')
        _LOADED_LIBRARIES.add(name)
    # Relative paths are anchored to the app, independent of Jupyter's cwd.
    path = Path(data_file).expanduser()
    if not path.is_absolute():
        path = APP_ROOT / path
    path = path.resolve()
    if not path.is_file():
        raise FileNotFoundError(f'Edit DATA_FILE in the notebook to select a current {profile} ROOT file. Missing: {path}')
    file = ROOT.TFile.Open(str(path))
    if not file or file.IsZombie():
        raise RuntimeError(f'Cannot open {path}')
    tree = file.Get('events')
    branch = 'sampic_event' if profile == 'SAMPIC' else 'nalu_event'
    if not tree or not tree.GetBranch(branch):
        file.Close()
        raise RuntimeError(f'{path} does not contain the {profile} events tree')
    print(f'{profile}: {path}\n{tree.GetEntries():,} entries; ROOT {ROOT.gROOT.GetVersion()}')
    return file, tree, str(path)


def present(tree, name):
    return bool(tree.GetBranch('has_sampic_'+name)) and bool(getattr(tree, 'has_sampic_'+name))


def hit_waveform(hit):
    """Copy samples out of ROOT ownership. Display calibrated data in mV."""
    values = np.array(hit.corrected_waveform, dtype=float, copy=True)
    return values * 1000 if hit.adc_corrected else values


def hit_baseline(hit):
    wf = hit_waveform(hit)
    return float(wf[:8].mean()) if wf.size else float('nan')


def hit_amplitude(hit):
    wf = hit_waveform(hit)
    return float(np.ptp(wf)) if wf.size else float('nan')


def hit_peak(hit):
    wf = hit_waveform(hit)
    return float(wf.max()) if wf.size else float('nan')


def waveform_entries(tree):
    result = []
    for idx in range(tree.GetEntries()):
        tree.GetEntry(idx)
        if present(tree, 'event') and any(len(h.corrected_waveform) for h in tree.sampic_event.hits):
            result.append(idx)
    return result


def sampic_unit(tree):
    """Aggregate plots must not mix calibrated mV and uncalibrated ADC counts."""
    modes = set()
    for idx in range(tree.GetEntries()):
        tree.GetEntry(idx)
        if present(tree, 'event'):
            modes.update(bool(h.adc_corrected) for h in tree.sampic_event.hits if len(h.corrected_waveform))
    if len(modes) > 1:
        raise ValueError('This file mixes calibrated and uncalibrated waveforms. Split by adc_corrected before aggregating; mV and ADC counts cannot be combined.')
    return 'mV' if modes == {True} else 'ADC counts' if modes else 'no waveform samples'


def integer(value):
    return ord(value) if isinstance(value, str) else int(value)


def waveform_packets(event, waveform):
    """Match a waveform by BOTH channel and timestamp, never parser index.

    Returned pointers are valid only until the next tree.GetEntry call.
    Packet order is acquisition order, not physical-window sorted order.
    """
    key = (integer(waveform.channel_num), int(waveform.trigger_time))
    return [p for p in event.packets.packets
            if (integer(p.channel), int(p.trigger_time)) == key]


def sampic_event_bytes(tree):
    """Reconstruct current uncompressed MIDAS bytes (one frontend per event).

    Includes 16-byte event header, 8-byte bank-area header, 12-byte BANK32 headers,
    and 8-byte payload alignment. Excludes unknown/unexposed banks and ODB records.
    """
    sizes = []
    if present(tree,'event'): sizes.append(12 + 154*len(tree.sampic_event.hits))
    if present(tree,'hit_metadata'): sizes.append(8 + 32*len(tree.sampic_hit_metadata.hits))
    for name,size in [('event_timing',56),('collector_timing',32),('readout_tick',56),('time_bin',32),('trigger_group',36)]:
        if present(tree,name): sizes.append(size)
    if present(tree,'trigger_records'): sizes.append(4 + 20*len(tree.sampic_trigger_records.triggers))
    return 24 + sum(12 + ((size+7)//8)*8 for size in sizes)
