"""Resume-aware genre runs, using two independent measurement processes (not AI agents)."""
import argparse
import concurrent.futures
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
GENRES = ("Boom Bap", "Trap", "Techno", "DnB")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("calibrate", "check"))
    parser.add_argument("material")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    output = ROOT / "docs" / "reports" / "presets"
    output.mkdir(parents=True, exist_ok=True)
    logs = ROOT / "build" / "presets"
    logs.mkdir(parents=True, exist_ok=True)
    def run(genre):
        slug = genre.lower().replace(" ", "_")
        result = output / f"{args.mode}_{slug}.json"
        with (logs / f"{args.mode}_{slug}.log").open("w") as log:
            code = subprocess.run([str(ROOT / "build/Release/HamloVerb_PresetLab.exe"), args.mode,
                args.material, str(ROOT / "build/preset-cache"), str(result), genre], cwd=ROOT,
                stdout=log, stderr=subprocess.STDOUT, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0)).returncode
        print(f"{genre}: exit {code}", flush=True)
        return code, result
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1,min(4,args.jobs))) as pool:
        results = list(pool.map(run, GENRES))
    rows = []
    hashes = set()
    for code, result in results:
        if result.exists():
            report = json.loads(result.read_text(encoding="utf-8"))
            rows += report["results"]
            hashes.add(report["dspHash"])
    if len(hashes) != 1 or len(rows) != 200:
        raise RuntimeError(f"Incomplete/inconsistent run: {len(rows)} presets, {len(hashes)} DSP hashes")
    passed = sum(bool(row["pass"]) for row in rows)
    combined = dict(mode=args.mode, dspHash=next(iter(hashes)), passed=passed, failed=200-passed,
                    presets={row["id"]:row for row in rows})
    (output / f"{args.mode}_all.json").write_text(json.dumps(combined,indent=2)+"\n",encoding="utf-8")
    md = [f"# HamloVerb preset {args.mode}", "", f"DSP/source fingerprint: `{combined['dspHash']}`.", "",
          f"**{passed}/200 passed.** Complete renders include tails; depth is wet/dry energy ratio.", "",
          "| Preset | Depth dB | Insert dBTP | Send dBTP | Unique | State | Result |",
          "|---|---:|---:|---:|---|---|---|"]
    for row in rows:
        md.append(f"| {row['name']} | {row['measuredDepthDb']:.3f} | {row['insertTruePeakDb']:.3f} | {row['sendTruePeakDb']:.3f} | {row['unique']} | {row['stateRoundTrip']} | {'PASS' if row['pass'] else 'FAIL'} |")
    (output / f"{args.mode}_all.md").write_text("\n".join(md)+"\n",encoding="utf-8")
    print(f"{passed}/200 passed",flush=True)
    return 0 if passed==200 and all(code==0 for code,_ in results) else 1

if __name__ == "__main__":
    raise SystemExit(main())
