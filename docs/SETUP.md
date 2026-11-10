# ft-ota environment setup

## Host (verified in sandbox: Ubuntu 24.04, gcc 13, py 3.12, cmake 4.x)
Arch/Omarchy:
    sudo pacman -S --needed base-devel git cmake ninja python python-pip nginx
    pip install --user pytest hypothesis gcovr      # or a venv
Verify: `make test` -> 3 C tests + 4 py tests green, core/ coverage printed.

## ESP-IDF (NOT verified here; do on your machine)
Deps (Arch): git wget flex bison gperf python cmake ninja ccache libffi libusb dfu-util
    mkdir -p ~/esp && cd ~/esp
    git clone --recursive --branch <TAG> https://github.com/espressif/esp-idf.git
    cd esp-idf && ./install.sh esp32s3 && . ./export.sh
Pin <TAG> to one stable release; record it in docs/ADR-009-idf-pin.md (open question in handoff doc).
Never touch eFuse features (secure boot v2, flash enc release, anti-rollback, JTAG disable) on your only board.

## Rule
core/ has no ESP-IDF dependency. Flash goes through tiny HAL.
