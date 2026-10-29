"""Record explicit recipe changes against the preserved first recipe bank."""
import json
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[1]
before = json.loads((ROOT / "docs/reports/presets/recipes_before_repair.json").read_text(encoding="utf-8"))
recipes = runpy.run_path(str(ROOT / "Tools/genre_presets.py"))["RECIPES"]
after = {genre + "/" + group + "/" + name: {"depth": depth, "model": model, "sampler": sampler, **extra}
         for genre, rows in recipes.items() for group, name, depth, model, sampler, extra in rows}
assert before.keys() == after.keys()
rows = []
for key in before:
    changes = [f"{field}: {before[key].get(field, 'default')} -> {after[key].get(field, 'default')}"
               for field in sorted(before[key].keys() | after[key].keys())
               if before[key].get(field) != after[key].get(field)]
    if changes:
        rows.append("| " + key + " | " + "; ".join(changes) + " |")
header = f"# HamloMag recipe repairs\n\n{len(rows)} recipes changed after the first complete calibrated check.\n\n"
header += "Aliases are the recipe fields in Tools/genre_presets.py; default means the parameter default.\n"
header += "DRIVE is recalibrated separately; see calibration_final.tsv. Depth labels and acceptance bands are unchanged.\n\n"
header += "| Recipe | Changes from the first recipe bank |\n|---|---|\n"
(ROOT / "docs/reports/presets/recipe_changes.md").write_text(header + "\n".join(rows) + "\n", encoding="utf-8")
print(f"{len(rows)} recipe changes recorded")
