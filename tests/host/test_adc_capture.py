"""Test real API frames and serial capture cleanup without physical hardware."""
from contextlib import redirect_stdout
import io
import itertools
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

RUNTIME = sys.argv.pop(1)
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "311-adc"))
import get_telemetry as tm
import rc


class CaptureTests(unittest.TestCase):
    def test_api_errors_preserve_running_stream_and_stop(self):
        frames = ["help\r", "tm_start 50\r", "tm_start 5\r", "tm_start 5x\r",
                  "tm_start 10 20\r", "tm_start -1\r", "tm_start 4294967296\r",
                  "tm_stop\r", "get_temp\r", "get_voltage\r", "charge_on\r",
                  "charge_off\r", "profiling\r", "tm_start\r", "tm_stop\r"]
        # Send errors between samples so command echo does not share a telemetry line.
        script = "".join("%d %s\n" % (i * 200000 + (10000 if i >= 2 else 0), f.encode().hex())
                         for i, f in enumerate(frames))
        output = subprocess.run([RUNTIME], input=script, text=True, capture_output=True, check=True).stdout
        self.assertEqual(output.count("error: usage tm_start [<period_ms>]"), 4)
        self.assertEqual(output.count("error: period_ms must be at least 10"), 1)
        self.assertIn("telemetry: started, period 100 ms", output)
        self.assertIn("charge: on", output)
        self.assertIn("charge: off", output)
        self.assertIn("telemetry  count", output)
        rows = [(float(t), line) for t, line in (r.split("\t", 1) for r in output.splitlines())]
        samples = [tm.parse_record(line) for t, line in rows if 0.2 <= t < 1.4 and line.startswith("tm: ")]
        self.assertGreater(len(samples), 20)
        self.assertTrue(all(b[0] - a[0] == 50000 for a, b in zip(samples, samples[1:])))
        self.assertFalse(any(line.startswith("tm:") for t, line in rows if 1.411 <= t < 2.6))
        self.assertIn("temp:", output)
        self.assertIn("voltage:", output)

    def test_parser_and_csv_reject_broken_data(self):
        for line in ("tm: 1 nan 27", "tm: -1 1.0 27", "tm: 1 2 inf", "tm: 1 2", "help"):
            self.assertIsNone(tm.parse_record(line))
        self.assertEqual(tm.parse_record("tm: 123 1.652 27.4"), (123, 1.652, 27.4))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "records.csv"
            tm.save_csv([(123, 1.652, 27.4), (50123, 1.650, 27.0)], path, "simulation")
            self.assertEqual(tm.load_csv(path)[0], [0, 0.05])
            self.assertEqual(tm.data_source(path), "simulation")
            path.write_text("time_s,voltage_v,temp_c\n0,1,27\n0,2,27\n", encoding="utf-8")
            with self.assertRaises(ValueError):
                tm.load_csv(path)

    def test_serial_cleanup_on_interrupt_and_read_failure(self):
        class Port:
            def __init__(self, failure):
                self.sent, self.failure = [], failure
            def __enter__(self): return self
            def __exit__(self, *args): pass
            def reset_input_buffer(self): pass
            def flush(self): pass
            def write(self, value): self.sent.append(value)
            def read(self, count): raise self.failure
        for failure in (KeyboardInterrupt(), OSError("USB disconnected")):
            port = Port(failure)
            timer = itertools.count(0, 0.01)
            with patch.object(tm.serial, "Serial", return_value=port), \
                 patch.object(tm.time, "sleep"), \
                 patch.object(tm.time, "monotonic", side_effect=lambda: next(timer)), \
                 redirect_stdout(io.StringIO()):
                if isinstance(failure, OSError):
                    with self.assertRaises(OSError): tm.collect("fake", 1, 10, [(0, "charge_on")])
                else:
                    self.assertEqual(tm.collect("fake", 1, 10, [(0, "charge_on")]), [])
            self.assertIn(b"charge_on\r", port.sent)
            self.assertEqual(port.sent[-2:], [b"tm_stop\r", b"charge_off\r"])

    def test_tau_from_known_curve(self):
        times = [i / 100 for i in range(1101)]
        peak = 3.3 * (1 - math.exp(-5))
        voltage = [0 if t < .5 else 3.3 * (1 - math.exp(-min(t - .5, 5)))
                   if t <= 5.5 else peak * math.exp(-(t - 5.5)) for t in times]
        found = rc.measure_tau(times, voltage)
        for key in ("charge", "discharge"):
            start, end, _ = found[key]
            # The specified 2%/98% thresholds and finite 5-tau charge bias this estimate slightly.
            self.assertAlmostEqual(end - start, 1, delta=.05)
        self.assertEqual(rc.rc_plan(1), (10, [(.5, "charge_on"), (5.5, "charge_off")], 11))


if __name__ == "__main__":
    unittest.main()
