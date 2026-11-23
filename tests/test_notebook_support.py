"""Semantic checks for notebook calculations against real serialized banks.
Run: python -m unittest discover -s tests -p test_notebook_support.py
"""
from pathlib import Path
import sys
import unittest
import numpy as np

APP = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(APP/'notebooks'))
from notebook_support import open_data, sampic_event_bytes, waveform_packets, integer, hit_waveform
from verify_current_banks import events


class NotebookSupportTests(unittest.TestCase):
    def test_midas_size_matches_wire(self):
        for run in ('00266','00267'):
            with self.subTest(run=run):
                path=APP/'.artifacts/bank_format_verification'/('run'+run)/'output.root'
                if not path.exists(): self.skipTest('Run real-file verification first')
                f,tree,_=open_data('SAMPIC',path)
                try:
                    count=0
                    for i,(_,banks) in enumerate(events(APP/'data'/('run'+run+'.mid.lz4'))):
                        tree.GetEntry(i)
                        expected=24+sum(12+((len(payload)+7)//8)*8 for payload in banks.values())
                        self.assertEqual(sampic_event_bytes(tree),expected)
                        count+=1
                    self.assertEqual(count,tree.GetEntries())
                finally:f.Close()

    def test_batched_waveform_packet_membership(self):
        path=APP/'.artifacts/bank_format_verification/run121479/output.root'
        if not path.exists(): self.skipTest('Run real-file verification first')
        f,tree,_=open_data('HDSoC',path)
        try:
            # Include every group in the first batch, which contains many triggers
            # per channel: the previous channel-only matching fails this check.
            tree.GetEntry(0);event=tree.nalu_event
            self.assertGreater(len(event.waveforms.waveforms),2)
            matched=0
            for wf in event.waveforms.waveforms:
                packets=waveform_packets(event,wf)
                self.assertEqual(len(packets)*32,len(wf.trace))
                self.assertEqual(len(packets),integer(event.header.num_windows))
                self.assertTrue(all(integer(p.channel)==integer(wf.channel_num) and int(p.trigger_time)==int(wf.trigger_time) for p in packets))
                matched+=len(packets)
            self.assertEqual(matched,len(event.packets.packets))
        finally:f.Close()

    def test_display_samples_are_copied_and_scaled(self):
        class Hit:
            adc_corrected=True
            corrected_waveform=[-.002, .1]
        hit=Hit()
        values=hit_waveform(hit)
        np.testing.assert_allclose(values,[-2,100])
        hit.corrected_waveform[0]=55
        np.testing.assert_allclose(values,[-2,100])
        hit.adc_corrected=False
        np.testing.assert_allclose(hit_waveform(hit),[55,.1])

if __name__=='__main__':unittest.main()
