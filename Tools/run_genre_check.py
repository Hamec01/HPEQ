"""Run the same preset checks by genre, retaining cross-genre duplicate detection.

Usage: python -I Tools/run_genre_check.py <material-directory> <report.md>
"""
import concurrent.futures
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
GENRES = ("Boom Bap", "Trap", "Techno", "DnB")


def run(genre, material, report_dir):
    stem = genre.replace(" ", "_")
    report = report_dir / (stem + ".md")
    with (report_dir / (stem + ".log")).open("w", encoding="utf-8") as log:
        result = subprocess.run(
            [str(ROOT / "build/Release/HamloMag_PresetLab.exe"), "check", material, str(report), genre + "/"],
            cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
        )
    content = report.read_text(encoding="utf-8")
    summary = re.search(r"(\d+) genre presets, (\d+) outside", content)
    if not summary or int(summary[1]) != 50 or result.returncode != int(summary[2]):
        raise RuntimeError(f"Incomplete or crashed check: {genre}, exit {result.returncode}")
    rows = [line for line in content.splitlines() if re.match(r"\| \d+ \|", line)]
    if len(rows) != 50:
        raise RuntimeError(f"Incomplete table: {genre}")
    print(f"{genre}: {50 - int(summary[2])} / 50 pass", flush=True)
    return int(summary[2]), rows


def main():
    material = str(Path(sys.argv[1]).resolve())
    report = Path(sys.argv[2]).resolve()
    report.parent.mkdir(parents=True, exist_ok=True)
    raw = ROOT / "build" / (report.stem + "_genres")
    raw.mkdir(parents=True, exist_ok=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        jobs = [pool.submit(run, genre, material, raw) for genre in GENRES]
        results = [job.result() for job in jobs]
    bad = sum(item[0] for item in results)
    rows = [row for _, table in results for row in table]
    summary = f"200 genre presets, {bad} outside their band / peak / level / uniqueness criteria"
    header = "| # | Genre / group / preset | Material | Depth | Saturation share | Out peak | RMS out - in | Pass |\n|---|---|---|---|---|---|---|---|"
    report.write_text("# HamloMag genre presets on their material\n\n" + summary + "\n\n" + header + "\n" + "\n".join(rows) + "\n", encoding="utf-8")
    print(summary, flush=True)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
