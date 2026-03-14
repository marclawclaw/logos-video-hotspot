#!/usr/bin/env bash
# Shared helpers for e2e tests

# The CLI binary is passed as $1 to each test script, or found in PATH
CLI="${CLI_BIN:-${1:-video-hotspot}}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
export QT_LOGGING_RULES="*.debug=false;*.warning=false"

# Use isolated test data dir
export XDG_DATA_HOME="${XDG_DATA_HOME:-$(mktemp -d /tmp/vh-e2e-XXXXXX)}"
export VH_E2E_TMPDIR="${XDG_DATA_HOME}"

PASS=0
FAIL=0
TEST_ERRORS=()

e2e_pass() {
    echo "  ✓ $1"
    PASS=$((PASS + 1))
}

e2e_fail() {
    echo "  ✗ $1"
    TEST_ERRORS+=("$1")
    FAIL=$((FAIL + 1))
}

e2e_require() {
    local desc="$1"; shift
    if "$@" > /dev/null 2>&1; then
        e2e_pass "$desc"
    else
        e2e_fail "$desc: command failed: $*"
    fi
}

# Assert jq expression is truthy
e2e_json_assert() {
    local desc="$1"
    local json="$2"
    local expr="$3"
    if command -v jq > /dev/null 2>&1; then
        if echo "$json" | jq -e "$expr" > /dev/null 2>&1; then
            e2e_pass "$desc"
        else
            e2e_fail "$desc (JSON assertion failed: $expr on: $json)"
        fi
    else
        # Fallback: grep for key strings
        if echo "$json" | grep -q "${expr//\"/}"; then
            e2e_pass "$desc"
        else
            e2e_fail "$desc (grep assertion failed: $expr on: $json)"
        fi
    fi
}

e2e_summary() {
    echo ""
    echo "Results: ${PASS} passed, ${FAIL} failed"
    if [ ${FAIL} -gt 0 ]; then
        for err in "${TEST_ERRORS[@]}"; do
            echo "  FAIL: $err"
        done
        exit 1
    fi
    exit 0
}

# Create a fake video file for testing
make_test_video() {
    local path="$1"
    local size="${2:-1024}"
    dd if=/dev/urandom of="$path" bs=1 count="$size" 2>/dev/null
    echo "$path"
}
