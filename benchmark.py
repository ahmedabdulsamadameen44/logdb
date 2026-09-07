# AI assistance (Claude, Anthropic): Claude wrote this benchmark script
# end-to-end at my request, for one-time performance comparison data
# used in the README. Not part of the graded application logic.

import sqlite3
import subprocess
import time
import csv

EXE_PATH = "./bin_logdb.exe"
CSV_PATH = "./data.csv"


def build_sqlite_db():
    conn = sqlite3.connect(":memory:")
    conn.execute("CREATE TABLE logs (host TEXT, path TEXT, status INTEGER, bytes INTEGER)")
    with open(CSV_PATH, newline="", encoding="utf-8", errors="ignore") as f:
        reader = csv.reader(f)
        next(reader)  # skip header
        rows = [(r[1], r[4], int(r[5]), int(r[6])) for r in reader if len(r) >= 7]
    conn.executemany("INSERT INTO logs (host, path, status, bytes) VALUES (?, ?, ?, ?)", rows)
    conn.execute("CREATE INDEX idx_status ON logs(status)")
    conn.commit()
    return conn, len(rows)


def time_cengine(query_string, capture=True):
    start = time.perf_counter()
    result = subprocess.run([EXE_PATH, "query", query_string], capture_output=capture, text=True)
    elapsed = time.perf_counter() - start
    line_count = result.stdout.count("\n") if capture else None
    return elapsed, line_count


def time_sqlite(conn, sql):
    start = time.perf_counter()
    rows = conn.execute(sql).fetchall()
    return time.perf_counter() - start, len(rows)


def main():
    print("Loading CSV into in-memory SQLite...")
    conn, total_rows = build_sqlite_db()
    print(f"Total rows loaded: {total_rows}\n")

    # First: how many rows actually match status=200? This tells us selectivity.
    _, count200 = time_sqlite(conn, "SELECT * FROM logs WHERE status = 200")
    pct = 100 * count200 / total_rows
    print(f"status=200 matches {count200} of {total_rows} rows ({pct:.1f}%)\n")

    print(f"{'Query':<20}{'C engine (s)':<15}{'C rows':<10}{'SQLite (s)':<15}{'SQLite rows':<12}")

    tests = [
        ("status = 200", "SELECT * WHERE status = 200", "SELECT * FROM logs WHERE status = 200"),
        ("bytes > 5000", "SELECT * WHERE bytes > 5000", "SELECT * FROM logs WHERE bytes > 5000"),
    ]

    for label, c_query, sqlite_query in tests:
        c_time, c_rows = time_cengine(c_query)
        sqlite_time, sqlite_rows = time_sqlite(conn, sqlite_query)
        print(f"{label:<20}{c_time:<15.4f}{c_rows:<10}{sqlite_time:<15.4f}{sqlite_rows:<12}")

    conn.close()


if __name__ == "__main__":
    main()
