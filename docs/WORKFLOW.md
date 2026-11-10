# ft-ota workflow: VM + Windows split

## Roles
| Where | What |
| --- | --- |
| Omarchy VM (VMware Workstation) | cmake, gcc, pytest, Hypothesis, gcovr, fuzzers (L6), nginx + fault-injecting server, tc netem. Layers L1, L2, L3 (server side), L5 host parts, L6. |
| Windows host | ESP-IDF 6.1 (installed via eim): build device firmware, flash, serial monitor. P4 HIL power-cut harness (pytest + pyserial) and the power-cut rig. |

No USB passthrough. Board stays on Windows so the HIL loop has no VM USB layer in the middle.

## One repo, two checkouts
- Single git repo. Clone it once in the VM, once on Windows. Sync with push/pull.
- Do not edit one working tree from both sides through a VMware shared folder (CRLF and file-locking problems).
- `.gitattributes` forces LF. On Windows also run: `git config core.autocrlf input`.
- Rule: a test result only counts for the commit it ran on. Record `git rev-parse --short HEAD` next to every result (CSV row, log). Flash the same commit you tested.

## core/ is shared code
- `core/` has no ESP-IDF dependency. Tested in the VM (host build, ASan/UBSan, gcov), compiled by ESP-IDF on Windows for the device.
- Flash access goes through the HAL. Host uses the simulator, device uses the real driver.

## Network for device updates (P3+)
- Board on Wi-Fi must reach nginx in the VM. VMware network adapter: Bridged (not NAT), so the VM gets a LAN address.
- Give the VM a DHCP reservation or static IP. The device URL and the TLS cert name must match that address.
- Check Windows firewall / VMware bridge settings if the board cannot connect.

## Windows extras (small)
- ESP-IDF environment (eim).
- Python with pyserial and pytest, only for the P4 harness.

## Per-step loop
1. VM: write failing test, watch it fail, minimal code, `make test` (whole suite).
2. Commit, push.
3. Windows: pull, build device firmware, flash, run device-side checks.
4. Log results with commit hash.
