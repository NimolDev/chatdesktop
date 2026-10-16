# Message cache benchmark

Measured 2026-10-09 on the local macOS arm64 host, Qt 6.11.1, Release build.
Each case uses a unique synthetic cache and 20 measured samples per operation.
Reads use a warm filesystem cache but reopen and decrypt storage on every call.
Writes include insert and sync, exclude DTO serialization, and append 20 snapshots
starting from the listed snapshot count. Decode includes parsing the latest full
MessageDto and converting it to the domain response. Timing excludes network/UI.
Synthetic messages contain 256 ASCII characters plus Khmer text and an emoji.

| Messages per snapshot | Initial snapshots | Initial file bytes | Read median / p95 ms | Append median / p95 ms | Decode median ms |
| --- | --- | --- | --- | --- | --- |
| 20 | 1 | 20,480 | 62.18 / 71.37 | 63.79 / 65.16 | 0.20 |
| 20 | 10 | 167,936 | 61.39 / 62.50 | 60.31 / 63.76 | 0.19 |
| 20 | 50 | 823,296 | 61.74 / 65.77 | 63.69 / 64.97 | 0.19 |
| 200 | 1 | 131,072 | 60.09 / 63.46 | 63.07 / 65.77 | 1.65 |
| 200 | 10 | 1,273,856 | 61.36 / 65.02 | 61.94 / 63.77 | 1.65 |
| 200 | 50 | 6,352,896 | 70.23 / 73.05 | 65.88 / 69.40 | 1.71 |

Every scenario verified record counts and returned payloads. These are local
measurements, not cold-disk or concurrent-load results; p95 uses 20 samples.

Inspection shows each operation opens LocalStorage, derives its key using scrypt,
and scans the file. This likely accounts for much of the approximately 60 ms
baseline; the benchmark does not profile those phases separately. Read loads all
snapshots before the repository selects a page. Running these synchronous calls
on the GUI thread can cause visible stalls.

An initial run with similar names exposed a filename collision: FileNameHelper
passes QString::data() to HMAC with QString::size() as the byte count, although
QString uses UTF-16. Consequently only part of each name/key is hashed. This
benchmark uses random names to isolate scenarios; production code is unchanged.
Changing filename generation needs consideration of existing cache/auth filenames.

Run with a Release build and your Qt installation:

```sh
cmake -S tests/message_cache -B /tmp/chat-cache-perf -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/macos
cmake --build /tmp/chat-cache-perf --target benchmark_message_cache -j4
/tmp/chat-cache-perf/benchmark_message_cache
```

The executable uses a unique QStandardPaths test-mode application directory and
removes it on normal completion or a caught exception. It does not access the
application's real message cache.

## After the cache fix

The repository now uses `savePage` / `readPage` from an asynchronous QFuture
continuation. Each page is a separate encrypted store with one replaceable
record. Empty responses also replace stale content. Versioned message filenames
use full UTF-8 HMAC inputs; the original helper remains for compatibility with
existing authentication storage. Old message-cache files are not read because
colliding keys cannot be reliably attributed to a conversation; successful
network fetches populate the new cache.

Updated Release run (20 samples, 200 messages per page):

| Cached pages | Read one page median / p95 ms | Replace page median / p95 ms |
| --- | --- | --- |
| 1 | 62.98 / 63.85 | 62.94 / 64.24 |
| 10 | 60.10 / 63.63 | 60.25 / 63.72 |
| 50 | 61.13 / 62.25 | 60.73 / 62.61 |

The single page file was 253,952 bytes after 20 replacements in every case.
A read of all 50 snapshots in the comparison API took 69.69 ms median.
The fixed per-operation cost remains; the change avoids GUI-thread blocking and
history-dependent scans rather than eliminating key derivation. The cache is
bounded per page, not by a global account quota. The initial portion of this run
overlapped a chat-target build; treat small timing differences as noise.

Validation: cache regression tests passed (replacement, page isolation, clearing,
account/name collision cases, UTF-8 names, and existing persistence/error tests).
The application's `chat` target also compiled successfully.

## Single conversation file and reusable handles

The current implementation stores page roots (`message-page-N`) inside one
`messages-v3-<hash>` conversation file, with no page suffix on disk. A mutex guards
an LRU pool of at most eight open LocalStorage instances. Writes still call sync;
repeated calls reuse the derived key and in-memory index. Logout calls closeAll.
Previous layouts are refetched rather than migrated because old filenames could
collide. Generic append/read APIs and indexed page APIs share the same store.

Latest Release run, 20 samples, warm handles, 200 messages per page:

| Indexed pages | Read one page median / p95 ms | Replace page median / p95 ms |
| --- | --- | --- |
| 1 | 0.120 / 0.127 | 0.298 / 0.327 |
| 10 | 0.120 / 0.146 | 0.304 / 0.421 |
| 50 | 0.119 / 0.132 | 0.291 / 0.319 |

These timings exclude first-open key derivation and scanning, which still occur
on a cold open or after eviction. They also exclude DTO decoding (~1.65 ms).
Comparison append records and page records coexist in this benchmark, so the
reported total conversation file sizes include both workloads. The chat build
ran concurrently with this benchmark; exact timings are machine/load dependent.
Regression tests cover root persistence across reopen, separate pages, a single
conversation filename, replacement, eviction, clearing and existing error cases.
