# Опыт с RC-цепью на приборе 311-adc. По номиналам деталей ведёт опыт, по 5τ на заряд
# и разряд, сохраняет записи в rc.csv, график в rc.png, номиналы и найденную τ в rc.log
# и сверяет τ с R·C. Приём потока устроен как в get_telemetry.py из п3.1.4.
#
#   python rc.py --r 10 --c 100   — опыт с RC-цепью, R в кОм, C в мкФ
#   python rc.py --plot rc.csv    — разбор готовой записи, номиналы берутся из rc.log

import argparse
import math

import matplotlib
from get_telemetry import collect, save_csv, load_csv, find_board, data_source

# окно не нужно: график сразу сохраняется в файл
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# настоящая τ отличается от R·C из-за допуска деталей: C ±20 %, R ±5 %
TAU_LOW = 0.8 * 0.95
TAU_HIGH = 1.2 * 1.05

def rc_plan(tau):
    """Период телеметрии, команды опыта и его длительность по ожидаемой τ."""
    # около ста отсчётов на τ, но не чаще, чем умеет прибор
    period_ms = max(10, round(tau * 10))
    # полсекунды покоя, 5τ заряда, 5τ разряда и полсекунды в запас
    actions = [(0.5, "charge_on"), (0.5 + 5 * tau, "charge_off")]
    return period_ms, actions, 1 + 10 * tau


def crossing(voltages, start, level, rising):
    """Номер первой записи от start, на которой напряжение перешло уровень."""
    for i in range(start, len(voltages)):
        if (voltages[i] >= level) if rising else (voltages[i] <= level):
            return i
    return None


def measure_tau(times, voltages):
    """τ заряда и разряда по уровням напряжения, как по триггеру осциллографа."""
    top = max(voltages)
    peak = voltages.index(top)
    result = {"top": top}
    # заряд: от начала подъёма (2 % вершины) до 63 % вершины
    start = crossing(voltages, 0, 0.02 * top, True)
    end = crossing(voltages, start, 0.632 * top, True) if start is not None else None
    if end is not None and end <= peak:
        result["charge"] = (times[start], times[end], voltages[end])
    # разряд: от начала спада (98 % вершины) до 37 % вершины
    start = crossing(voltages, peak, 0.98 * top, False)
    end = crossing(voltages, start, 0.368 * top, False) if start is not None else None
    if end is not None:
        result["discharge"] = (times[start], times[end], voltages[end])
    return result


def analyze_rc(csv_name, r_kohm, c_uf):
    tau = r_kohm * c_uf / 1000
    low, high = TAU_LOW * tau, TAU_HIGH * tau
    times, voltages, _ = load_csv(csv_name)
    found = measure_tau(times, voltages)
    top = found["top"]

    fig, ax = plt.subplots(figsize=(8, 4.5))
    ax.plot(times, voltages, label="напряжение на конденсаторе")
    ax.axhline(0.632 * top, color="gray", linestyle="--", linewidth=1)
    ax.axhline(0.368 * top, color="gray", linestyle="--", linewidth=1)
    ax.text(times[-1], 0.632 * top, "63 %", va="bottom", ha="right", color="gray")
    ax.text(times[-1], 0.368 * top, "37 %", va="bottom", ha="right", color="gray")

    source = data_source(csv_name)
    lines = ["data_source = " + source, "R_kohm = %g" % r_kohm, "C_uf = %g" % c_uf, "tau_expected_s = %.3f" % tau,
             "tau_min_s = %.3f" % low, "tau_max_s = %.3f" % high]
    passed = True
    for key, name in (("charge", "заряда"), ("discharge", "разряда")):
        if key not in found:
            print("τ %s: уровень не пройден, проверьте цепь и номиналы" % name)
            lines.append("tau_%s_s = none" % key)
            passed = False
            continue
        start, end, level = found[key]
        measured = end - start
        inside = low <= measured <= high
        passed = passed and inside
        # от начала и конца заряда отсчитывается ожидаемый момент перехода уровня
        ax.axvline(start, color="tab:purple", linestyle="--", linewidth=1,
                   label="начало и конец заряда" if key == "charge" else None)
        ax.axvspan(start + low, start + high, color="tab:green", alpha=0.15,
                   label="ожидаемый переход уровня" if key == "charge" else None)
        ax.plot([end], [level], "o", color="tab:red",
                label="уровень пройден" if key == "charge" else None)
        print("τ %s: %.3f с, ожидается %.2f–%.2f с, %s" %
              (name, measured, low, high, "в диапазоне" if inside else "вне диапазона"))
        lines.append("tau_%s_s = %.3f" % (key, measured))
    lines.append("result = %s" % ("ok" if passed else "fail"))

    ax.set_title(("Учебная модель, без платы\n" if source == "simulation" else "") +
                 "R = %g кОм, C = %g мкФ, ожидаемая τ = %.2f с" % (r_kohm, c_uf, tau))
    ax.set_xlabel("Время, с")
    ax.set_ylabel("Напряжение на GP26, В")
    ax.grid(True)
    ax.legend(loc="upper right")
    fig.tight_layout()
    fig.savefig("rc.png", dpi=150)
    plt.close(fig)
    print("График сохранён в rc.png")

    with open("rc.log", "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("Номиналы и τ записаны в rc.log")
    print("Запись сошлась с расчётом" if passed else "Запись не сошлась с расчётом")
    return passed


def read_log(name):
    """Номиналы R и C из rc.log прошлого опыта или None, если файла нет."""
    values = {}
    try:
        with open(name, encoding="utf-8") as f:
            for line in f:
                key, _, value = line.partition("=")
                values[key.strip()] = value.strip()
        return float(values["R_kohm"]), float(values["C_uf"])
    except (OSError, KeyError, ValueError):
        return None


def main():
    parser = argparse.ArgumentParser(description="Опыт с RC-цепью на приборе 311-adc")
    parser.add_argument("--r", type=float, help="сопротивление резистора, кОм")
    parser.add_argument("--c", type=float, help="ёмкость конденсатора, мкФ")
    parser.add_argument("--port", help="COM-порт вместо автоматического поиска")
    parser.add_argument("--plot", metavar="CSV", help="разобрать готовую запись опыта, без платы")
    args = parser.parse_args()

    if (args.r is None) != (args.c is None):
        parser.error("нужны оба номинала, --r и --c")
    if args.r is None:
        # без номиналов можно только разобрать запись, номиналы — из прошлого опыта
        found = read_log("rc.log") if args.plot else None
        if found is None:
            parser.error("укажите номиналы деталей: --r <кОм> --c <мкФ>")
        args.r, args.c = found

    if not all(map(math.isfinite, (args.r, args.c))) or not 1 <= args.r <= 100 or args.c <= 0:
        parser.error("R от 1 до 100 кОм, C положительная и конечная")
    tau = args.r * args.c / 1000
    if not math.isfinite(tau) or tau < 0.2 or round(tau * 10) > 0xFFFFFFFF:
        parser.error("R·C должно быть не меньше 0,2 с и помещаться в период прибора")
    print("Ожидаемая τ = R·C = %.3f с" % tau)

    if args.plot:
        if not analyze_rc(args.plot, args.r, args.c):
            parser.exit(1, "Постоянная времени вне допуска или не найдена.\n")
    else:
        board = find_board() if not args.port else None
        device = args.port or (board.device if board else None)
        if not device:
            parser.exit(1, "Плата не найдена. Подключите Pico с прошивкой 311-adc или укажите --port COMn.\n")
        period_ms, actions, seconds = rc_plan(tau)
        print("Плата на порту %s, опыт %.1f с, период %d мс" % (device, seconds, period_ms))
        # Discharge even a previously charged capacitor before starting the record.
        records = collect(device, seconds, period_ms, actions, settle_s=5 * tau)
        if not records:
            parser.exit(1, "Ни одной строки телеметрии не принято.\n")
        save_csv(records, "rc.csv")
        print("Записей %d, сохранены в rc.csv" % len(records))
        if not analyze_rc("rc.csv", args.r, args.c):
            parser.exit(1, "Постоянная времени вне допуска или не найдена.\n")


if __name__ == "__main__":
    main()
