"""Run all five practical captures with the real board; stop on missing data."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time

from get_telemetry import find_board

ROOT = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description="Снять реальные результаты заданий 3.1.1–3.1.5")
    parser.add_argument("--r", type=float, default=10, help="резистор, кОм; по умолчанию 10")
    parser.add_argument("--c", type=float, default=100, help="конденсатор, мкФ; по умолчанию 100")
    args = parser.parse_args()
    if find_board() is None:
        parser.exit(1, "Pico не найдена. Загрузите firmware/311_adc.uf2 и закройте терминалы COM-порта.\n")
    for number in (1, 2, 3):
        script = "check-3-1-%d.py" % number
        log = ROOT / ("device-3-1-%d.log" % number)
        started = time.time()
        subprocess.run([sys.executable, str(ROOT / script)], cwd=ROOT, check=True)
        if not log.exists() or log.stat().st_mtime < started - 1:
            parser.exit(1, "Скрипт не создал свежий лог: " + str(log) + "\n")
        if "SIMULATED" in log.read_text(encoding="utf-8"):
            parser.exit(1, "Вместо нового измерения остался учебный лог.\n")
        if number == 1 and "project: 311-adc" not in log.read_text(encoding="utf-8"):
            parser.exit(1, "На плате другая прошивка: загрузите firmware/311_adc.uf2.\n")
    for command in (["get_telemetry.py"], ["rc.py", "--r", str(args.r), "--c", str(args.c)]):
        subprocess.run([sys.executable, *command], cwd=ROOT, check=True)
    for name in ("telemetry", "rc"):
        metadata = json.loads((ROOT / (name + ".meta.json")).read_text(encoding="utf-8"))
        if metadata.get("source") != "hardware":
            parser.exit(1, "Данные %s не заменены измерениями платы.\n" % name)
    print("Готово: три лога, два CSV, два PNG и rc.log записаны с реальной платы.")
    print("Теперь проверьте графики и отправьте изменённые результаты в GitHub.")


if __name__ == "__main__":
    main()
