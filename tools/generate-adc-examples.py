"""Reproducible examples from production C code and a labelled ADC/RC model.

These outputs are NOT board measurements. Real capture overwrites them.
"""
import argparse
import ast
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "311-adc"
sys.path.insert(0, str(PROJECT))
from get_telemetry import parse_record, save_csv, plot
from rc import analyze_rc


def run(runtime, frames, rc=False):
    data = "".join("%d %s\n" % (round(t * 1e6), frame.encode("ascii").hex()) for t, frame in frames)
    result = subprocess.run([str(runtime)] + (["--rc"] if rc else []), input=data,
                            text=True, capture_output=True, check=True)
    return [(float(t), line) for t, line in (row.split("\t", 1) for row in result.stdout.splitlines())]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    for number in (1, 2, 3):
        name = "3-1-%d" % number
        tree = ast.parse((PROJECT / ("check-" + name + ".py")).read_text(encoding="utf-8"))
        steps = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                     and any(isinstance(t, ast.Name) and t.id == "STEPS" for t in n.targets))
        frames, moment = [], 0.0
        for frame, delay in steps:
            frames.append((moment, frame))
            moment += delay
        output = run(runtime, frames + [(moment, "\r")])
        events = [(t, 0, "-->", frame.replace("\r", "")) for t, frame in frames]
        events += [(t, 1, "<--", line) for t, line in output]
        header = [
            "источник: учебная программная модель, без платы и физических измерений",
            "модель: настоящий C-код 311-adc; синтетические часы, ADC, GPIO и USB-идентичность",
            "задание: 3.1.%d" % number, "проект: 311-adc",
            "устройство: 2e8a:000a", "серийный номер: 0000000000000000",
            "порт: SIMULATED", "начало: условная эпоха воспроизводимого примера",
        ]
        lines = header + [("%8.3f %s %s" % (t, direction, line)).rstrip()
                          for t, _, direction, line in sorted(events, key=lambda event: event[:2])]
        lines.append("итог: отправлено кадров %d, принято строк %d" % (len(frames), len(output)))
        (PROJECT / ("device-" + name + ".log")).write_text("\n".join(lines) + "\n", encoding="utf-8")
    for name, rc, frames in [
        ("telemetry", False, [(1, "tm_start\r"), (11, "tm_stop\r")]),
        ("rc", True, [(1, "tm_start 10\r"), (1.5, "charge_on\r"),
                      (6.5, "charge_off\r"), (12, "tm_stop\r")]),
    ]:
        records = [r for _, line in run(runtime, frames, rc) if (r := parse_record(line)) is not None]
        save_csv(records, PROJECT / (name + ".csv"), source="simulation")
        print("%s: %d model records" % (name, len(records)))
    plot(PROJECT / "telemetry.csv", PROJECT / "telemetry.png")
    # The course analysis writes rc.png and rc.log in its working directory.
    import os
    os.chdir(PROJECT)
    if not analyze_rc("rc.csv", 10, 100):
        raise SystemExit("RC model analysis failed")


if __name__ == "__main__":
    main()
