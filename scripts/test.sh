#!/usr/bin/env bash
#
# Builds and runs each kernel test in QEMU, then reports PASS/FAIL.
#
#   ./scripts/test.sh              run every test
#   ./scripts/test.sh fork signal  run only the given tests
#
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

ALL_TESTS=(pmm vmm malloc fork wait_exit signal panic panic_exception)
LOG_DIR="build/test-logs"
TESTS=("$@")
[ ${#TESTS[@]} -eq 0 ] && TESTS=("${ALL_TESTS[@]}")

# Required tools
export PATH="$ROOT/toolchain/bin:$PATH"
command -v i686-elf-gcc >/dev/null || { echo "i686-elf-gcc not found"; exit 2; }
command -v nasm >/dev/null || { echo "nasm not found"; exit 2; }
command -v qemu-system-i386 >/dev/null || { echo "qemu-system-i386 not found"; exit 2; }

# Time limit in seconds for each test
timeout_of() {
    case "$1" in
        malloc)    echo 900 ;;
        wait_exit) echo 3600 ;;
        *)         echo 60 ;;
    esac
}

# Output that means the test has finished
stop_pattern() {
    case "$1" in
        panic|panic_exception) echo "Kernel panic" ;;
        *)                     echo "KFS_TEST_DONE|Kernel panic|Failed|segfault" ;;
    esac
}

# Decide the result from the log
check() {
    local t=$1 log=$2
    case "$t" in
        panic)
            grep -q "eax: 0 ebx: 1 ecx: 2 edx: 3" "$log" &&
            grep -q "esi: 4 edi: 5 ebp: 6" "$log" &&
            grep -q "Stack: a b c d e f" "$log" &&
            grep -q "Kernel panic: !!panic test!!" "$log"
            ;;
        panic_exception)
            grep -q "Kernel panic:" "$log"
            ;;
        *)
            grep -q "KFS_TEST_DONE" "$log" &&
            ! grep -qE "Failed|Kernel panic|segfault" "$log"
            ;;
    esac
}

run_test() {
    local t=$1
    local log="$LOG_DIR/$t.log"
    local limit; limit=$(timeout_of "$t")
    local pattern; pattern=$(stop_pattern "$t")

    printf "%-16s " "$t"

    if ! make -s TEST="$t" >"$log.build" 2>&1; then
        echo "BUILD FAILED (see $log.build)"
        return 1
    fi

    # Boot the test kernel headless; kernel output goes to the log via port 0xE9
    : >"$log"
    local start=$SECONDS
    qemu-system-i386 -kernel "build/test-$t/kernel.elf" -m 1G \
        -display none -no-reboot -monitor none -serial none \
        -debugcon "file:$log" 2>"$log.qemu" &
    local qemu=$!

    # The kernel never exits on its own, so stop QEMU once the test is over
    while kill -0 "$qemu" 2>/dev/null; do
        if grep -qE "$pattern" "$log"; then
            sleep 1            # let the rest of the output reach the log
            break
        fi
        [ $((SECONDS - start)) -ge "$limit" ] && break
        sleep 0.5
    done
    kill "$qemu" 2>/dev/null
    wait "$qemu" 2>/dev/null

    if check "$t" "$log"; then
        echo "PASS ($((SECONDS - start))s)"
        return 0
    fi
    echo "FAIL (see $log)"
    return 1
}

mkdir -p "$LOG_DIR"
failed=()
for t in "${TESTS[@]}"; do
    run_test "$t" || failed+=("$t")
done

echo
if [ ${#failed[@]} -eq 0 ]; then
    echo "all ${#TESTS[@]} tests passed"
    exit 0
fi
echo "${#failed[@]} of ${#TESTS[@]} tests failed: ${failed[*]}"
exit 1