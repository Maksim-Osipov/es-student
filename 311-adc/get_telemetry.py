"""Collect 311-adc USB telemetry or plot a saved CSV without a board."""
import argparse
import csv
import json
import math
from pathlib import Path
import time

import matplotlib
import serial
from serial.tools import list_ports

matplotlib.use("Agg")
import matplotlib.pyplot as plt

VENDOR_ID = 0x2E8A
PRODUCT_ID = 0x000A


def find_board():
    return next((p for p in list_ports.comports()
                 if p.vid == VENDOR_ID and p.pid == PRODUCT_ID), None)


def parse_record(line):
    parts = line.split()
    if len(parts) != 4 or parts[0] != "tm:":
        return None
    try:
        stamp, voltage, temp = int(parts[1]), float(parts[2]), float(parts[3])
    except ValueError:
        return None
    if stamp < 0 or not all(map(math.isfinite, (voltage, temp))):
        return None
    return stamp, voltage, temp


def collect(device, seconds, period_ms, actions=(), settle_s=0.1):
    """Schedule commands by host clock; keep device timestamps for measurements."""
    if not math.isfinite(seconds) or seconds <= 0 or not 10 <= period_ms <= 0xFFFFFFFF:
        raise ValueError("seconds > 0; period must be 10..4294967295 ms")
    records = []
    pending = sorted(actions)
    with serial.Serial(device, baudrate=115200, timeout=0.01, write_timeout=1) as port:
        time.sleep(0.2)
        try:
            # Clear any previous stream and start an RC experiment discharged.
            port.write(b"tm_stop\r")
            if actions:
                port.write(b"charge_off\r")
            time.sleep(settle_s)
            port.reset_input_buffer()
            port.write(("tm_start %d\r" % period_ms).encode("ascii"))
            started = time.monotonic()
            received = ""
            while time.monotonic() - started < seconds:
                while pending and time.monotonic() - started >= pending[0][0]:
                    port.write((pending.pop(0)[1] + "\r").encode("ascii"))
                received += port.read(256).decode("ascii", "replace")
                while "\n" in received:
                    line, received = received.split("\n", 1)
                    record = parse_record(line.strip())
                    if record is not None and (not records or record[0] > records[-1][0]):
                        records.append(record)
                        print("%.3f с  %.3f В  %.1f °C" % (record[0] / 1e6, *record[1:]))
                # Bound memory if a disconnected/broken sender never ends a line.
                if len(received) > 4096:
                    received = ""
        except KeyboardInterrupt:
            print("Остановлено по Ctrl+C")
        finally:
            try:
                port.write(b"tm_stop\r")
            finally:
                if actions:
                    port.write(b"charge_off\r")
            port.flush()
    return records


def save_csv(records, name, source="hardware"):
    if not records:
        raise ValueError("Нет записей телеметрии")
    start_us = records[0][0]
    with open(name, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["time_s", "voltage_v", "temp_c"])
        for stamp, voltage, temp in records:
            writer.writerow(["%.6f" % ((stamp - start_us) / 1e6), "%.3f" % voltage, "%.1f" % temp])
    Path(name).with_suffix(".meta.json").write_text(
        json.dumps({"source": source, "description": (
            "Учебная модель, без платы и физических измерений" if source == "simulation"
            else "Телеметрия платы по USB")}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def data_source(name):
    try:
        return json.loads(Path(name).with_suffix(".meta.json").read_text(encoding="utf-8"))["source"]
    except (OSError, ValueError, KeyError):
        return "unspecified"


def load_csv(name):
    times, voltages, temps = [], [], []
    with open(name, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames != ["time_s", "voltage_v", "temp_c"]:
            raise ValueError("Нужны столбцы time_s,voltage_v,temp_c")
        for row in reader:
            t, v, temp = (float(row[k]) for k in reader.fieldnames)
            if not all(map(math.isfinite, (t, v, temp))) or t < 0 or (times and t <= times[-1]):
                raise ValueError("Неверные значения или немонотонное время в CSV")
            times.append(t)
            voltages.append(v)
            temps.append(temp)
    if not times:
        raise ValueError("CSV не содержит записей")
    return times, voltages, temps


def plot(csv_name, png_name="telemetry.png"):
    times, voltages, temps = load_csv(csv_name)
    fig, (top, bottom) = plt.subplots(2, 1, sharex=True, figsize=(9, 6))
    top.plot(times, voltages)
    top.set_ylabel("Напряжение на GP26, В")
    top.grid(True)
    bottom.plot(times, temps, color="tab:red")
    bottom.set_ylabel("Температура кристалла, °C")
    bottom.set_xlabel("Время, с")
    bottom.grid(True)
    source = data_source(csv_name)
    fig.suptitle("Телеметрия 311-adc" + (
        " — учебная модель, без платы" if source == "simulation" else ""))
    fig.tight_layout()
    fig.savefig(png_name, dpi=150)
    plt.close(fig)
    print("График сохранён в " + str(png_name))


def main():
    parser = argparse.ArgumentParser(description="Сбор телеметрии прибора 311-adc")
    parser.add_argument("--seconds", type=float, default=10)
    parser.add_argument("--period", type=int, default=100, help="период, мс")
    parser.add_argument("--port", help="COM-порт, если автоматический поиск не подходит")
    parser.add_argument("--plot", metavar="CSV", help="построить график без платы")
    args = parser.parse_args()
    if args.plot:
        plot(args.plot)
        return
    if not math.isfinite(args.seconds) or args.seconds <= 0 or not 10 <= args.period <= 0xFFFFFFFF:
        parser.error("seconds > 0, period от 10 до 4294967295 мс")
    board = find_board() if not args.port else None
    device = args.port or (board.device if board else None)
    if not device:
        parser.exit(1, "Плата не найдена. Подключите Pico с прошивкой 311-adc или укажите --port COMn.\n")
    records = collect(device, args.seconds, args.period)
    if not records:
        parser.exit(1, "Нет телеметрии. Проверьте прошивку и закройте другие терминалы COM-порта.\n")
    save_csv(records, "telemetry.csv")
    print("Записей %d, сохранены в telemetry.csv" % len(records))
    plot("telemetry.csv")


if __name__ == "__main__":
    main()
