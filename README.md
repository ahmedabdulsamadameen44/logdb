# logdb

A hand-built storage and indexing engine for querying NASA's 1995 HTTP
access logs, written in C and exposed through a Flask web app.

CS50 2026 final project by Ahmed.

## Why this exists

Most "build a database-backed web app" CS50 finals reach for SQLite and
stop. This project instead implements the storage engine itself — binary
row persistence, a custom hash index, and a hand-written SQL-flavored
query parser — in C, with Flask as a thin presentation layer on top. The
goal was to understand what SQLite is actually doing under the hood by
building a smaller version of it, not to outperform it (see
[Benchmarks](#benchmarks) for why it doesn't, and why that's informative
rather than a failure).

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
column lists, `OR`. The dataset is a single flat table, so joins don't
apply; the rest were cut to keep the parser's scope finishable in the
project timeline rather than half-implemented.

## Indexing

`load` builds a hash table keyed on `status` (`hashtable.c`), where each
bucket entry stores the list of row offsets matching that status value.
`index.dat` is this table serialized to disk. At query time, if a query
filters on `status`, the engine reads the index, gets the candidate row
offsets, and `fseek`s directly to each one instead of scanning every row.

## Benchmarks

Tested against SQLite (loaded with the same ~3M-row dataset) on identical
queries. SQLite wins across the board:

| Query | C engine | SQLite | SQLite advantage |
|---|---|---|---|
| `status=200` | 12.7458s (2,696,532 rows) | 2.7041s (2,696,600 rows) | ~4.7x |
| `bytes>5000` | 2.7080s (1,165,921 rows) | 1.1984s (1,165,956 rows) | ~2.3x |

Row counts differ slightly between engines on both queries (68 rows on
`status=200`, 35 rows on `bytes>5000`) — see
[Known issue](#known-issue-unresolved) below; the `bytes>5000` gap is the
one investigated in detail (35 raw rows → 26 unique tuples after
deduplication). The `status=200` gap hasn't been separately diffed.

See [Index selectivity](#index-selectivity-a-design-limitation-not-a-bug)
below for why SQLite wins on speed, and why that's a real finding rather
than just "my engine is worse."

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
  inputs; a `op[]` buffer was read via `strcmp` without being
  null-terminated first, causing nondeterministic comparison behavior.

## Known issue (unresolved)

**Row-count discrepancy vs. SQLite.** Cross-checking `bytes>5000` results
against the same query on SQLite shows a 35-row gap in raw row count,
narrowing to 26 rows when comparing unique `(host, path, status, bytes)`
tuples (i.e. ~9 of the 35 are duplicate rows on one side). Investigated and ruled out:
CSV malformation, buffer truncation in reads, the signed-integer-overflow
bug above (fixed, but didn't close this gap), row loss during `load`
(confirmed: all rows are present in `rows.dat`), benchmark counting
artifacts, and index-side deduplication (disproved — the raw,
non-deduplicated query output contains far more duplicate lines than 26,
so the index isn't silently collapsing rows). Root cause not identified
before submission. Next step if revisited: diff the two unique result
sets directly to name the specific missing rows, rather than comparing
counts.

Documented here rather than hidden, on the view that an honest known-gap
with a documented investigation trail is more useful — and more
representative of real engineering — than silence or a rushed, unverified
patch.

## Index selectivity: a design limitation, not a bug

The hash index on `status` actually *hurts* performance for
low-selectivity queries. `status=200` matches roughly 91% of the
dataset, so looking up its index entry means `fseek`ing to hundreds of
thousands of scattered row offsets — each a separate disk seek — which
loses to a plain sequential scan over the same rows. The index only pays
off when the filtered value is rare, since then it turns a full scan
into a handful of seeks.

SQLite's query planner would recognize low selectivity and fall back to
a sequential scan instead of using an index. This engine has no query
planner: it always uses the index when one exists for the filtered
field, whether or not that's actually the better plan. That's the
biggest engineering takeaway from this project — an index isn't free,
and knowing when *not* to use one is as much a part of query execution
as building the index in the first place.

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

## AI assistance disclosure

Claude (Anthropic) was used as a coding assistant throughout this
project — reviewing code for bugs, helping design some function/struct
shapes, and helping debug specific failures. Per-file attribution
comments describing exactly what was AI-assisted are in the source
files themselves (see comment blocks at the top of each `.c`/`.h`/`.py`
file). The core logic, architecture decisions, and query engine
implementation are my own work; Claude's role was closer to code review
and pair debugging than authorship.
