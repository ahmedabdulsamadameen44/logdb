# logdb

A hand-built storage and indexing engine for querying NASA's 1995 HTTP
access logs, written in C and exposed through a Flask web app.

CS50 2026 final project.

## Why this exists

Most "build a database-backed web app" CS50 finals reach for SQLite and
stop. This project instead implements the storage engine itself — binary
row persistence, a custom hash index, and a hand-written SQL-flavored
query parser — in C, with Flask as a thin presentation layer on top. The
goal was to understand what SQLite is actually doing under the hood by
building a smaller version of it, not to outperform it (see
[Benchmarks](#benchmarks) for how close it gets, and why the remaining
gap is informative rather than a failure).

## Video demo
https://youtu.be/vMdCyZDsZ4U?si=ee4BddG77PdJp0Zs

## Architecture

```
CSV file  →  [C engine: load]  →  rows.dat + index.dat
                                         ↓
                              [C engine: query]  →  stdout (\x1F-delimited)
                                         ↓
                          [Flask: bridge.py subprocess call]
                                         ↓
                              [Flask: query builder UI]
```

**C core** (`main.c`, `command.c`, `hashtable.c`, `index.c`, `row.c`, `parser.c`, `query_parser.c`)
Compiled to a standalone binary invoked with two subcommands:

- `logdb load <csv_path>` — parses the CSV, writes each row as a
  fixed-size binary record to `rows.dat`, and builds a hash index on the
  `status` field, written to `index.dat`.
- `logdb query "<query string>"` — parses a SQL-flavored query string,
  runs it against `rows.dat` (using `index.dat` to narrow the scan when
  the query filters on `status`), and prints results to stdout.

**Flask app** (`app.py`, `bridge.py`, templates)
Handles user accounts, session-based auth, a visual query builder, saved
queries, and log upload. Flask's *own* metadata (users, saved queries)
lives in a small SQLite database (`logdb.db`) — SQLite here is just
Flask-app plumbing, not the log-query engine. The actual log data never
touches SQLite.

**The bridge** (`bridge.py`)
Flask shells out to the compiled C binary via `subprocess.run()` and
parses its stdout. Fields are delimited with `\x1F` (ASCII Unit
Separator) rather than a comma or pipe, specifically so that user data
containing those characters (a URL with a comma, say) can't collide with
the delimiter.

## Query language

A small SQL-inspired grammar, hand-tokenized and hand-parsed
(`query_parser.c`) — not a wrapper around real SQL syntax. Supported
forms:

```
SELECT * WHERE field=value
SELECT * WHERE field>value AND field<=value
SELECT COUNT field WHERE field=value
SELECT COUNT field WHERE field=value GROUP BY field
SELECT COUNT field GROUP BY field ASC|DESC
```

Fields: `status`, `bytes`, `host`, `path`.
Operators: `=`, `>`, `<`, `>=`, `<=`.
Conditions combine with `AND` only.

**Deliberately unsupported:** joins, subqueries, arbitrary `SELECT`
column lists, `OR`, `LIMIT`/pagination. The dataset is a single flat
table, so joins don't apply; the rest were cut to keep the parser's
scope finishable in the project timeline rather than half-implemented.
The missing `LIMIT` in particular means an unfiltered `SELECT *` (or any
very low-selectivity query) returns every matching row with no cap —
for the full ~3M-row dataset that can mean millions of rows rendered
into one HTML response through the web UI. A real query engine needs
pagination; this one doesn't have it.

## Indexing

`load` builds a hash table keyed on `status` (`hashtable.c`), where each
bucket entry stores the list of row offsets matching that status value.
`index.dat` is this table serialized to disk. At query time, if a query
filters on `status`, the engine reads the index and gets the candidate
row offsets for that value.

## Query execution: from per-row seeks to a bulk read

The first working version of `query()` read one row at a time directly
off disk — for the indexed path, that meant `fseek`ing to each candidate
row offset and issuing a separate `fread` per row; for the unindexed
fallback (`bytes`, `host`, `path`), a sequential `read_row()` call per
row. Correct, but slow on `status=200`, where 90.9% of the dataset
matches: 2.7 million individual seek-and-read calls.

Two things were tried, in order, to speed this up:

**Sorting candidate row offsets before seeking (reverted).** The
hypothesis was that seeking in ascending order would let the OS page
cache serve nearby reads without hitting disk on every call, since
insertion order (and therefore the unsorted candidate list) has no
relationship to on-disk position. Implemented with `qsort()` and a
larger stdio buffer (`setvbuf`, 1MB). Measured result: no improvement —
the `status=200` query became slow enough to look hung during testing.
Reverted. The likely explanation: `rows.dat` is around 594MB, larger
than what fits usefully in page cache on top of everything else running,
so "more sequential-looking" seek order didn't translate into fewer
actual disk hits — the bottleneck was seek *count*, not seek *order*.

**Reading the entire file into memory once per query (kept).** Instead
of `fseek`/`fread` per row, `query()` now reads all of `rows.dat` into
one heap-allocated buffer with a single `fread` at the start, then
indexes into it directly by row number (`&all_rows[row_number]`) —
pointer arithmetic, no further syscalls. This applies to both the
indexed path and the linear-scan fallback, so both benefited. Cost:
one ~594MB allocation and read per `query()` invocation, since each
query runs as a fresh, short-lived process (no persistent server holding
the file in memory between queries) — but that upfront cost turned out
to be far cheaper than millions of separate syscalls. See
[Benchmarks](#benchmarks) for the measured before/after.

## Benchmarks

Tested against SQLite (loaded with the same ~3M-row dataset) on identical
queries and an identical load operation. Numbers below reflect the
current bulk-read `query()` implementation.

**Load:**

| | Time |
|---|---|
| C engine (`load`, writes to disk) | 4.28s |
| SQLite (bulk insert, in-memory) | 7.49s |

The C engine loads faster despite writing `rows.dat`/`index.dat` to
actual disk, while SQLite here is in-memory only — so this comparison
already favors the C engine less than it could; a disk-backed SQLite
comparison would likely be slower still, which would widen the gap
further. Worth noting for context: profiling `load` in isolation showed
~4.4s wall time against ~0.03s of combined user+sys CPU time, meaning
the operation is dominated by disk I/O, not by the parsing or hashing
logic itself.

**Query:**

| Query | C engine | SQLite | SQLite advantage |
|---|---|---|---|
| `SELECT *` (no filter) | 4.93s (2,965,561 rows) | 2.80s (2,965,561 rows) | ~1.8x |
| `status=200` | 4.78s (2,696,532 rows) | 2.69s (2,696,600 rows) | ~1.8x |
| `bytes>5000` | 2.35s (1,165,921 rows) | 1.20s (1,165,956 rows) | ~2.0x |

Before the bulk-read change, `status=200` took 12.18s — the switch from
per-row `fseek`/`fread` to one bulk read cut that to 4.78s, a ~2.5x
improvement, and brought its SQLite gap down from ~4.2x to ~1.8x —
roughly in line with the other two queries. That convergence is itself
informative: once the per-row syscall overhead is gone, the remaining
gap looks uniform across filtered and unfiltered queries alike, which
points to a baseline difference in I/O and scan efficiency between raw
`fread` and SQLite's own storage engine, rather than anything specific
to indexing or query shape.

`SELECT *` with no `WHERE` clause returns identical row counts on both
engines (2,965,561 — the full dataset), confirming there's no row loss
on the unfiltered case; the discrepancies below are specific to the two
filtered queries.

Row counts differ slightly between engines on the two filtered queries
(68 rows on `status=200`, 35 rows on `bytes>5000`) — see
[Known issue](#known-issue-unresolved) below; the `bytes>5000` gap is the
one investigated in detail (35 raw rows → 26 unique tuples after
deduplication). The `status=200` gap hasn't been separately diffed.

## Bugs found and fixed

- **Signed integer overflow in comparator.** `row_matches()` originally
  derived comparison sign via subtraction (`row->bytes - val`), which is
  undefined behavior in C on overflow. Replaced with explicit three-way
  comparison (`>`, `<`, else `==`).
- **Missing header includes (recurring, 4+ occurrences).** `index.h`
  missing `<stdio.h>` for `FILE`, `index.c` missing `<stdlib.h>` for
  `malloc`/`free`, `parser.c` missing `<stdlib.h>` for `atoi`. Caught by
  `-Wall`, not by a runtime failure — a reminder that relying on
  transitive includes is fragile even when it happens to compile.
- **Test fixture format mismatch.** An early hand-typed test log used
  raw Apache log format while `parse_line()` expected CSV, causing a
  silent all-rows-skipped failure: `parse_line()` returned 1 on every
  line, `load()` exited 0 having written zero rows, and nothing
  surfaced an error. Fixed by generating a real CSV-format fixture from
  the actual dataset (`sample_csv.log`).
- **Duplicate Flask route registration.** A leftover stub `/saved` route
  was left in place after the real one was added, causing a Flask
  `AssertionError` at startup from two view functions mapped to the same
  endpoint.
- **Tokenizer bugs (`query_parser.c`).** An infinite loop occurred when
  the operator-token branch failed to advance the scan index on certain
  inputs; an `op[]` buffer was read via `strcmp` without being
  null-terminated first, causing nondeterministic comparison behavior.
- **Double-free risk in an early bulk-read draft.** A bounds check added
  inside the per-row loop (skipping out-of-range row offsets) originally
  called `free()` on the index entry's `key`/`rows` pointers from inside
  that inner loop — memory the outer loop still needed and would free
  again itself, a double-free. Caught before it shipped; fixed by
  skipping the single bad row with `continue` instead of freeing
  anything mid-loop.

## Known issue (unresolved)

**Row-count discrepancy vs. SQLite.** Cross-checking `bytes>5000` results
against the same query on SQLite shows a 35-row gap in raw row count,
narrowing to 26 rows when comparing unique `(host, path, status, bytes)`
tuples (i.e. ~9 of the 35 are duplicate rows on one side). Investigated
and ruled out: CSV malformation, buffer truncation in reads, the
signed-integer-overflow bug above (fixed, but didn't close this gap),
row loss during `load` (confirmed: all rows are present in `rows.dat`),
benchmark counting artifacts, and index-side deduplication (disproved —
the raw, non-deduplicated query output contains far more duplicate
lines than 26, so the index isn't silently collapsing rows). Root cause
not identified before submission. Next step if revisited: diff the two
unique result sets directly to name the specific missing rows, rather
than comparing counts.

Documented here rather than hidden, on the view that an honest known gap
with a documented investigation trail is more useful — and more
representative of real engineering — than silence or a rushed, unverified
patch.

## Index selectivity: a design limitation

The hash index on `status` still doesn't help — and structurally can't —
for low-selectivity queries, independent of the seek-versus-bulk-read
question above. `status=200` matches roughly 91% of the dataset, so the
index narrows almost nothing: looking it up still means processing
nearly every row either way. The index only pays off when the filtered
value is rare, since then it turns "look at everything" into "look at a
handful of rows."

SQLite's query planner would recognize low selectivity and choose a
sequential scan over an index automatically. This engine has no query
planner: it always uses the index when one exists for the filtered
field, whether or not that's actually the better plan for that
particular value. That's the biggest engineering takeaway from this
project — an index isn't free, and knowing when *not* to use one is as
much a part of query execution as building the index in the first
place.

## Setup

```
# build the C engine
make

# load a dataset
./bin_logdb load data.csv

# run the Flask app
flask run
```

Requires a `data.csv` in NASA-access-log CSV format (columns: index,
host, time, method, url, response, bytes). See `sample_csv.log` for the
expected format.

Loading the full dataset writes an uncompressed, fixed-width `rows.dat`
(`sizeof(Row)` per row — currently 200 bytes) to disk; for the full
~3M-row dataset that's roughly 600MB of free disk space required. This
is the direct cost of the fixed-width format, which is what makes
constant-time random access by row number possible in the first place —
a variable-length format would be smaller on disk but would rule out
direct seeking.

## AI assistance disclosure

Claude (Anthropic) was used as a coding assistant throughout this
project — reviewing code for bugs, helping design some function/struct
shapes, and helping debug specific failures. Per-file attribution
comments describing exactly what was AI-assisted are in the source
files themselves (see comment blocks at the top of each `.c`/`.py`
file). The core logic, architecture decisions, and query engine
implementation are original work; Claude's role was closer to code
review, adding tests, and pair debugging than authorship.