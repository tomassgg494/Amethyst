#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

BIN=./amethystc
PASS=0
FAIL=0

check() {
  local name="$1"
  local expected="$2"
  local actual="$3"
  if [[ "$actual" == "$expected" ]]; then
    PASS=$((PASS + 1))
    echo "  PASS  $name"
  else
    FAIL=$((FAIL + 1))
    echo "  FAIL  $name"
    echo "        expected: $(printf %q "$expected")"
    echo "        actual:   $(printf %q "$actual")"
  fi
}

mkdir -p build

echo "== compile + run examples =="

# hello
$BIN -o build/hello examples/hello.amt
check "hello output" "42
1
0" "$(./build/hello)"

# fib: print fib(0..15), exit code = fib(20) = 6765 (mod 256 = 201? bash $? is 0-255)
# fib(20)=6765, 6765 % 256 = 6765 - 26*256 = 6765 - 6656 = 109
$BIN -o build/fib examples/fib.amt
expected_fib=$(python3 - <<'PY'
def fib(n):
    a,b=0,1
    for _ in range(n):
        a,b=b,a+b
    return a
print("\n".join(str(fib(i)) for i in range(16)))
PY
)
check "fib stdout" "$expected_fib" "$(./build/fib; true)"
./build/fib >/dev/null && rc=0 || rc=$?
# run with exit code capture
set +e
./build/fib >/dev/null
rc=$?
set -e
check "fib exit (fib20 mod 256)" "109" "$rc"

# control
$BIN -o build/control examples/control.amt
set +e
out=$(./build/control)
rc=$?
set -e
# acc = 10+8+6+4+2 = 30
expected_control="30
-1
0
1
1
0"
check "control output" "$expected_control" "$out"
check "control exit" "30" "$rc"

# params
$BIN -o build/params examples/params.amt
set +e
out=$(./build/params)
rc=$?
set -e
expected_params="28
1
0"
check "params output" "$expected_params" "$out"
check "params exit" "24" "$rc"  # 280 % 256 = 24

# call
$BIN -o build/call examples/call.amt
set +e
out=$(./build/call)
rc=$?
set -e
check "call output" "5" "$out"
check "call exit" "5" "$rc"

# loops (for / break / continue)
$BIN -o build/loops examples/loops.amt
set +e
out=$(./build/loops)
rc=$?
set -e
expected_loops="45
7
5
21
6"
check "loops output" "$expected_loops" "$out"

# arrays (index / assign / inference)
$BIN -o build/arrays examples/arrays.amt
set +e
out=$(./build/arrays)
rc=$?
set -e
expected_arrays="10
50
0
99
219
16
42
1"
check "arrays output" "$expected_arrays" "$out"

# strings
$BIN -o build/strings examples/strings.amt
set +e
out=$(./build/strings)
rc=$?
set -e
expected_strings='Hello, Amethyst!
string with	tab and "quotes"
line1
line2
n is:
7
positive'
check "strings output" "$expected_strings" "$out"

# runtime bounds check (index OOB → exit 1 + message)
mkdir -p build
cat > build/oob.amt <<'EOF'
fn main() -> int {
    var a: int[3] = [1, 2, 3];
    print(a[5]);
    return 0;
}
EOF
$BIN -o build/oob build/oob.amt
set +e
err=$(./build/oob 2>&1)
rc=$?
set -e
if [[ $rc -eq 1 && "$err" == *"index 5 out of bounds"* ]]; then
  PASS=$((PASS + 1))
  echo "  PASS  runtime bounds check"
else
  FAIL=$((FAIL + 1))
  echo "  FAIL  runtime bounds check (rc=$rc)"
  echo "        err: $err"
fi

echo "== error cases =="

expect_fail() {
  local name="$1"
  local file="$2"
  local needle="$3"
  set +e
  err=$($BIN -o build/should_fail "$file" 2>&1)
  rc=$?
  set -e
  if [[ $rc -ne 0 && "$err" == *"$needle"* ]]; then
    PASS=$((PASS + 1))
    echo "  PASS  $name"
  else
    FAIL=$((FAIL + 1))
    echo "  FAIL  $name (rc=$rc)"
    echo "        err: $err"
  fi
}

expect_fail "type mismatch" tests/err_type.amt "cannot initialize 'int x' with value of type 'bool'"
expect_fail "undeclared var" tests/err_undeclared.amt "assignment to undeclared variable 'y'"
expect_fail "arity" tests/err_arity.amt "expects 1 argument(s), got 2"
expect_fail "missing return" tests/err_missing_return.amt "not all control paths"
expect_fail "redeclare fn" tests/err_redeclare_fn.amt "redefinition of function 'main'"
expect_fail "redeclare var" tests/err_redeclare_var.amt "redeclaration of 'x'"
expect_fail "syntax" tests/err_syntax.amt "expected"
expect_fail "cond not bool" tests/err_cond_type.amt "if condition must be bool"
expect_fail "break outside loop" tests/err_break_outside.amt "'break' outside of a loop"
expect_fail "index non-array" tests/err_index_nonarray.amt "cannot index into a value of type 'int'"
expect_fail "array size mismatch" tests/err_array_size.amt "array size mismatch"
expect_fail "array elem type" tests/err_array_elem_type.amt "array element 2 has type 'bool'"
expect_fail "string in var" tests/err_string_var.amt "cannot store a string"

echo
echo "passed: $PASS  failed: $FAIL"
[[ $FAIL -eq 0 ]]
