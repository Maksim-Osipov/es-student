"""Package tracked sources and a freshly built UF2 without adding binaries to Git."""
import argparse
import hashlib
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    firmware = ROOT / "311-adc/build/311_adc.uf2"
    if not firmware.is_file():
        parser.error("Build 311-adc first: missing " + str(firmware))
    paths = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT).decode("utf-8").split("\0")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for name in paths:
            if name and (ROOT / name).is_file():
                archive.write(ROOT / name, name)
        archive.write(firmware, "311-adc/firmware/311_adc.uf2")
        digest = hashlib.sha256(firmware.read_bytes()).hexdigest()
        archive.writestr("311-adc/firmware/SHA256SUMS", digest + "  311_adc.uf2\n")
        archive.writestr("PACKAGE.txt", "Source revision: " + revision + "\n"
                         "Ready firmware: 311-adc/firmware/311_adc.uf2\n"
                         "Instructions: 311-adc/README.md\n"
                         "Included initial ADC/RC datasets are SIMULATION, not board measurements.\n")
    print(args.output.resolve())


if __name__ == "__main__":
    main()
