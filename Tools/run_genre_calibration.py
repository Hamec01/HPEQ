"""Run independent genre calibration jobs and combine only a complete successful result.

Usage: python -I Tools/run_genre_calibration.py <material-directory> [keys.txt]
The executable must be built first. Raw logs remain in build/calibration/.
"""
import concurrent.futures
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
GENRES = ("Boom Bap", "Trap", "Techno", "DnB")


def run(genre, material, keys):
    log = ROOT / "build" / "calibration" / (genre.replace(" ", "_") + ".tsv")
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("w", encoding="utf-8") as output:
        result = subprocess.run(
            [str(ROOT / "build/Release/HamloMag_PresetLab.exe"), "calibrate", material,
             ";".join(keys) if keys is not None else genre + "/"],
            cwd=ROOT, stdout=output, stderr=subprocess.STDOUT,
        )
    rows = [line for line in log.read_text(encoding="utf-8").splitlines() if line.startswith("CAL\t")]
    expected = len(keys) if keys is not None else 50
    if result.returncode or len(rows) != expected:
        raise RuntimeError(f"{genre}: exit {result.returncode}, {len(rows)} / {expected}; see {log}")
    print(f"{genre}: {expected} / {expected} calibrated", flush=True)
    return rows


def main():
    material = str(Path(sys.argv[1]).resolve())
    selected = Path(sys.argv[2]).read_text(encoding="utf-8").splitlines() if len(sys.argv) > 2 else None
    saved = ROOT / "Tools/genre_presets_cal.tsv"
    previous = {row.split("\t")[1]: row for row in saved.read_text(encoding="utf-8").splitlines()
                if row.startswith("CAL\t")} if selected is not None else {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        # Balance repair subsets across workers: one genre can contain most of the failed recipes.
        jobs = ([pool.submit(run, genre, material, None) for genre in GENRES] if selected is None else
                [pool.submit(run, f"repair_{i + 1}", material, selected[i::4])
                 for i in range(min(4, len(selected)))])
        rows = [row for job in jobs for row in job.result()]
    if selected is not None:
        assert {row.split("\t")[1] for row in rows} == set(selected)
        previous.update({row.split("\t")[1]: row for row in rows})
        rows = list(previous.values())
    assert len({row.split("\t")[1] for row in rows}) == 200
    (ROOT / "Tools/genre_presets_cal.tsv").write_text("\n".join(rows) + "\n", encoding="utf-8")
    print("200 / 200 calibration records saved; regenerate and run the independent check next.", flush=True)


if __name__ == "__main__":
    main()
