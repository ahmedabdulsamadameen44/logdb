# AI assistance (Claude, Anthropic): helped fix PROJECT_ROOT path bug
# after file move, and helped write run_load().



import subprocess
import os
import subprocess

# src/ is one level below project root, where the exe and .dat files live
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
EXE_PATH = os.path.join(PROJECT_ROOT, "bin_logdb.exe")


def run_query(query_string):
    result = subprocess.run(
        [EXE_PATH, "query", query_string],
        capture_output=True,
        text=True,
        cwd=PROJECT_ROOT
    )

    if result.returncode != 0:
        return {"error": result.stderr.strip()}

    lines = result.stdout.strip().split("\n")
    if not lines or lines == [""]:
        return {"mode": "star", "rows": []}

    count_value = None
    groups = []
    star_rows = []

    for line in lines:
        fields = line.split("\x1F")
        if fields[0] == "COUNT":
            count_value = int(fields[1])
        elif fields[0] == "GROUP":
            groups.append({"value": fields[1], "count": int(fields[2])})
        else:
            star_rows.append({"host": fields[0], "path": fields[1], "status": fields[2], "bytes": fields[3]})

    if star_rows:
        return {"mode": "star", "rows": star_rows}

    result_dict = {"mode": "count", "value": count_value, "groups": groups}
    return result_dict





def run_load(csv_path):
    result = subprocess.run(
        [EXE_PATH, "load", csv_path],
        capture_output=True,
        text=True,
        cwd=PROJECT_ROOT
    )
    if result.returncode != 0:
        return {"error": result.stderr.strip() or "load failed"}
    return {"success": True}