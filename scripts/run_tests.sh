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

# compound assignment + len()
$BIN -o build/compound examples/compound.amt
set +e
out=$(./build/compound)
rc=$?
set -e
expected_compound="4
4
10
21
32
43
106"
check "compound output" "$expected_compound" "$out"

# array parameters (slices)
$BIN -o build/slices examples/slices.amt
set +e
out=$(./build/slices)
rc=$?
set -e
expected_slices="150
2
-1
1
5
15
3"
check "slices output" "$expected_slices" "$out"

# slice ABI: >6 integer args, and a slice mixed with scalars
cat > build/abi.amt <<'EOF'
fn seven(a: int, b: int, c: int, d: int, e: int, f: int, g: int) -> int {
    return a + b + c + d + e + f + g;
}

fn many(a: int[], b: int, c: int, d: int, e: int, f: int, g: int) -> int {
    var total = 0;
    for i in 0..len(a) {
        total += a[i];
    }
    return total + b + c + d + e + f + g;
}

fn fourAndSlice(a: int, b: int, c: int, d: int, s: int[]) -> int {
    var total = a + b + c + d;
    for i in 0..len(s) {
        total += s[i];
    }
    return total;
}

fn fiveAndSlice(a: int, b: int, c: int, d: int, e: int, s: int[]) -> int {
    var total = a + b + c + d + e;
    for i in 0..len(s) {
        total += s[i];
    }
    return total;
}

fn main() -> int {
    var s: int[3] = [10, 20, 30];
    print(seven(1, 2, 3, 4, 5, 6, 7));
    print(many(s, 1, 2, 3, 4, 5, 6));
    print(fourAndSlice(1, 2, 3, 4, s));
    print(fiveAndSlice(1, 2, 3, 4, 5, s));
    return 0;
}
EOF
$BIN -o build/abi build/abi.amt
set +e
out=$(./build/abi)
rc=$?
set -e
check "slice/stack argument ABI" "28
81
70
75" "$out"

# a slice is bounds-checked against its runtime length
cat > build/sliceoob.amt <<'EOF'
fn first(a: int[]) -> int {
    return a[5];
}

fn main() -> int {
    var s: int[3] = [1, 2, 3];
    print(first(s));
    return 0;
}
EOF
$BIN -o build/sliceoob build/sliceoob.amt
set +e
err=$(./build/sliceoob 2>&1)
rc=$?
set -e
if [[ $rc -eq 1 && "$err" == *"index 5 out of bounds for array of size 3"* ]]; then
  PASS=$((PASS + 1))
  echo "  PASS  runtime slice bounds check"
else
  FAIL=$((FAIL + 1))
  echo "  FAIL  runtime slice bounds check (rc=$rc)"
  echo "        err: $err"
fi

# warnings are reported on stderr but do not fail the compilation
cat > build/warn.amt <<'EOF'
fn main() -> int {
    var unused = 42;
    var x = 1;
    return x;
    print(999);
}
EOF
set +e
err=$($BIN -o build/warn build/warn.amt 2>&1)
rc=$?
set -e
if [[ $rc -eq 0 && "$err" == *"warning: unused variable"* && "$err" == *"warning: unreachable code"* ]]; then
  PASS=$((PASS + 1))
  echo "  PASS  sema warnings (non-fatal)"
else
  FAIL=$((FAIL + 1))
  echo "  FAIL  sema warnings (rc=$rc)"
  echo "        err: $err"
fi

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

# runtime division / modulo by zero → exit 1 + message
expect_divzero() {
  local name="$1"
  local body="$2"
  cat > build/divzero.amt <<EOF
fn main() -> int {
    var z = 0;
$body
    return 0;
}
EOF
  $BIN -o build/divzero build/divzero.amt
  set +e
  err=$(./build/divzero 2>&1)
  rc=$?
  set -e
  if [[ $rc -eq 1 && "$err" == *"division by zero"* ]]; then
    PASS=$((PASS + 1))
    echo "  PASS  $name"
  else
    FAIL=$((FAIL + 1))
    echo "  FAIL  $name (rc=$rc)"
    echo "        err: $err"
  fi
}

expect_divzero "runtime division by zero" "    var n = 10 / z;
    print(n);"
expect_divzero "runtime modulo by zero" "    var n = 10 % z;
    print(n);"
expect_divzero "runtime /= by zero" "    var n = 10;
    n /= z;
    print(n);"

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
expect_fail "compound on bool" tests/err_compound_bool.amt "'+=' expects int on both sides"
expect_fail "len of non-array" tests/err_len_nonarray.amt "'len' expects an array argument"
expect_fail "len arity" tests/err_len_arity.amt "'len' expects 1 argument(s), got 0"
expect_fail "len redefinition" tests/err_len_redef.amt "'len' is a builtin and cannot be redefined"
expect_fail "array literal argument" tests/err_array_arg_literal.amt "must be a variable (assign the array first)"
expect_fail "slice element type" tests/err_slice_elem_type.amt "expected 'bool[]', got 'int[]'"
expect_fail "sized array parameter" tests/err_param_array_size.amt "array parameters must be written as 'int[]'"
expect_fail "slice local variable" tests/err_slice_local.amt "local arrays need a fixed size"
expect_fail "array copy" tests/err_array_copy.amt "must be initialized with an array literal"

echo
echo "passed: $PASS  failed: $FAIL"
[[ $FAIL -eq 0 ]]
