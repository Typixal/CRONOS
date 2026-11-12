# CRONOS

Fault-tolerant delta OTA updates for the ESP32-S3.

CRONOS sends a device a small, signed, chunked patch instead of a whole firmware image. It applies the patch into the inactive A/B slot, can resume after a power cut or network loss, and only switches boot slots once the full result has been verified. The goal is a measured answer to one question: *how safely and cheaply can a constrained device take a delta update?*

> **Status: early development.** The design is complete and the host-side decoder is under construction. There is no device updater yet, and no measurements exist yet. See [Project status](#project-status) for exactly what works and what does not.

---

## Why this exists

A normal OTA downloads the whole image. A naive delta OTA saves bandwidth but usually has to restart from scratch if it is interrupted. CRONOS aims for three properties:

| Goal | Meaning |
| --- | --- |
| **G1** | An update never leaves the device unbootable. |
| **G2** | An interrupted update resumes with at most one chunk of re-download. |
| **G3** | The patch is much smaller than the full image. |
| **G4** | Every claim is backed by measured data (simulator and hardware). |
| **G5** | Updates are authenticated end to end. |

**What this project is not.** The mechanisms here (A/B slots, journaled recovery, chunked delta) all have prior art, including patents and recent papers. CRONOS is an implementation and evaluation on the ESP32-S3, not a claim of a new mechanism. Any result will be reported as a measured comparison against baselines such as stock `esp_https_ota` and detools-style delta OTA.

Also note: in an A/B layout a delta update still writes the full output image to the inactive slot, so delta saves *bandwidth*, not flash wear. Reducing erases needs compare-before-erase, and the actual saving will be measured, not assumed.

---

## How it works (design)

```
BUILD HOST              SERVER                 DEVICE (ESP32-S3)
+-------------+  files  +--------------+ HTTPS +-----------------------------+
| delta-forge |-------->| nginx static |<------| updater                     |
| (python)    | patch + | /fw/<b>-<t>/ | Range | - fetcher (esp_http_client) |
| sign key    | manifest| /latest.json |       | - verifier (SHA-256, ECDSA) |
+-------------+         +--------------+       | - patcher (per-chunk decode)|
                                               | - writer (skip-identical)   |
                                               | - state store (NVS)         |
                                               +-----------------------------+
TEST RIG: RPi/MCU + MOSFET power switch + UART harness (planned)
```

**Key decisions** (full ADR summaries live in the design doc):

- **A/B slots, base is immutable (ADR-001).** The running slot is never modified, so any crash leaves a bootable image. A patch is a pure function of (patch, base), which makes replay safe.
- **Stock bootloader, recovery logic in the app (ADR-002).** Uses ESP-IDF app rollback (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`) and does not fork the bootloader.
- **Verification-based resume (ADR-003).** Hashes in the signed manifest are the source of truth. The NVS journal is only a hint, so a lost or corrupt journal costs time, never correctness.
- **Chunked delta format (ADR-004).** Each output chunk has its own independently decodable patch segment, so the resume point is free.
- **HTTPS with Range requests (ADR-005)** and a **signed manifest, ECDSA P-256 (ADR-006).** The manifest lists a SHA-256 for every chunk's patch and output.
- **Full-image fallback (ADR-007)** after repeated delta failures or a base mismatch.
- **Compare-before-erase (ADR-008)** to skip sectors that already match.

**Per-chunk write protocol.** For each chunk: fetch the patch bytes, check the patch hash, decode, check the output hash, then for each 4 KB sector read, skip if identical, otherwise erase and program. Finally read back, verify, and record progress. Only after the whole target image hashes correctly does the updater flip `otadata` and reboot into the new slot, where a self-test must mark it valid or the bootloader rolls back.

### Patch segment format (work in progress)

One segment is decoded per output chunk. It is a stream of ops, all integers little-endian:

| Op | Byte | Header | Payload |
| --- | --- | --- | --- |
| `COPY` | `0x00` | 9 bytes: op, `u32 base_off`, `u32 len` | none; copies `len` bytes from base |
| `ADD` | `0x01` | 9 bytes: op, `u32 base_off`, `u32 len` | `len` diff bytes; `out[i] = (uint8_t)(base[base_off+i] + diff[i])` |
| `LIT` | `0x02` | 5 bytes: op, `u32 len` | `len` literal bytes |

This format is still evolving and is not yet specified in a `docs/FORMAT.md`. Per-chunk compression (heatshrink or zlib, to be decided by measurement) is planned but not implemented.

---

## Project status

Verified on the host as of commit `2e9c25d`:

- `make test` passes: 14 C test cases (3 in `test_chunk`, 11 in `test_patch`) and 4 Python tests, built with `-Wall -Wextra -Werror` plus ASan and UBSan.
- Line coverage of `core/src` measured at 97.9%, branch coverage 82.5%.

| Phase | Work | State |
| --- | --- | --- |
| **P0** | Toolchain, A/B partitions, stock `esp_https_ota` + rollback demo, GPIO flash trace | Host side done. Hardware partly done: ESP-IDF 6.1 builds and flashes the board, and a stock `simple_ota_example` with a custom 8 MB A/B table and rollback enabled runs on an ESP32-S3 N8R8. The local HTTPS OTA, the rollback proof and the GPIO trace are **not done**. |
| **P1** | Host decoder, delta-forge, round-trip tests, corpus | **In progress.** C decoder handles `COPY`, `ADD`, `LIT` with truncation and base-range checks. The Python forge, manifest, and round-trip test do not exist yet. |
| **P2** | Flash simulator, state machine, exhaustive crash tests | Not started |
| **P3** | Device updater, server, fault-injecting server | Not started |
| **P4** | Hardware power-cut rig, soak, fuzzing | Not started |
| **P5** | Baselines, experiments, write-up | Not started |

### Known limitations

- **The decoder is not safe for untrusted input yet.** It does not check `out_len`, so a segment can write past the output buffer. Output-size checks are the next planned step. Do not feed it data you do not control.
- An unknown op returns `FTOTA_ERR_UNIMPLEMENTED`; it will get its own status code.
- Nothing has been measured. There are no patch-size, flash-erase, resume-latency, or power-cut results, so no performance or reliability claims are made here.
- Nothing in this repo has been run on the device yet. Device-side behaviour described above is design intent.

---

## Quick start (host tests)

Requirements: a C11 compiler with ASan/UBSan (gcc or clang), CMake 3.20+, Python 3.11+, git. ESP-IDF is **not** needed for host tests.

```bash
git clone https://github.com/Typixal/CRONOS.git
cd CRONOS

python3 -m venv .venv
source .venv/bin/activate
pip install pytest hypothesis gcovr

make test        # C tests (ctest) then Python tests (pytest)
make cov         # coverage summary for core/src
make clean
```

Run `make test` inside the virtual environment. The Python tests use the system `python3`, which will not have `pytest` otherwise.

For the device side (ESP32-S3, ESP-IDF 6.1), see [`docs/SETUP.md`](docs/SETUP.md). The board is currently developed on Windows (build, flash, monitor) while host tests run on a Linux VM; [`docs/WORKFLOW.md`](docs/WORKFLOW.md) describes that split.

> **Warning:** do not enable eFuse-based features (secure boot v2, flash encryption in release mode, anti-rollback, JTAG disable) on your only dev board. eFuse burns are irreversible.

---

## Repository layout

```
CRONOS/
  core/            C library: decoder and chunk validation (no ESP-IDF dependency)
    include/       public headers (ftota_patch.h, ftota_chunk.h)
    src/           patch.c, chunk.c
    tests/         Unity tests
  forge/           delta-forge (Python): chunk math so far, tests in forge/tests
  docs/            SETUP.md, WORKFLOW.md
  third_party/unity/   vendored Unity test framework (v2.6.0)
  CMakeLists.txt, Makefile, pytest.ini
```

Planned, not yet present: `sim/` (flash simulator and crash injection), `device/` (ESP-IDF updater), `server/` (nginx config and fault-injecting server), `rig/` (power-cut harness), `corpus/` (firmware pairs), `results/` (logs and plots).

**Design rule:** `core/` has no ESP-IDF dependency. Flash access will go through a small HAL (read, erase sector, program page), with a simulator on the host and the real driver on the device. The same `core/` code is then tested on the host and compiled into the firmware.

---

## Testing approach

Development is test-first: write a failing test, watch it fail for the right reason, write the minimal code, run the whole suite, then commit.

The full test strategy (in the design doc) has six layers: host unit tests, a flash simulator that crashes at every flash operation, network fault injection, a hardware power-cut campaign, soak runs, and fuzzing. Only the first layer exists today. The system invariants those layers will check include: after any crash and reboot the boot selector yields a valid signed image, the running slot's bytes never change during an update, and `otadata` never selects an unverified image.

---

## Contributing

This is a personal project, but issues and suggestions are welcome. If you send a change:

1. Add or update a test first and confirm it fails before the fix.
2. Run the full `make test` inside the venv before committing.
3. Keep `core/` free of ESP-IDF headers.
4. Chunk sizes must be multiples of 4096 (the flash erase unit).
5. Label anything not run on real hardware as unverified.
6. Commit specific files rather than `git add -A`, so `.venv/` and `build/` never get committed.

A test or flash result only counts for the commit it ran on, so record `git rev-parse --short HEAD` next to results.

---

## Author

Ihsan Bin Sajid. This began as a final-year-project candidate and is now a personal engineering project.

## License

No license has been chosen yet. Until one is added, all rights are reserved. Vendored Unity in `third_party/unity` is MIT-licensed; see its `LICENSE.txt`.
