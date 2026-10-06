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
1
3.5
9.75
ametista
rocha"
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
v1.2
1
0
4
[amethyst]
[guest]
v1.3
positive'
check "strings output" "$expected_strings" "$out"

# floats
$BIN -o build/floats examples/floats.amt
set +e
out=$(./build/floats)
rc=$?
set -e
expected_floats='3.5
6
1
8.75
1.4
-3.5
1
1
1
12
2.5
5
3
2
-2
3.5'
check "floats output" "$expected_floats" "$out"

# IEEE-754: /0.0 gives inf/NaN instead of trapping, NaN is not equal to itself
cat > build/ieee.amt <<'EOF'
fn main() -> int {
    var nan = 0.0 / 0.0;
    print(1.0 / 0.0);
    print(-1.0 / 0.0);
    print(nan == nan);
    print(nan != nan);
    print(nan < 1.0);
    print(1.0 / 0.0 > 1.0);
    return 0;
}
EOF
$BIN -o build/ieee build/ieee.amt
set +e
out=$(./build/ieee)
rc=$?
set -e
check "IEEE-754 float edge cases" "inf
-inf
0
1
0
1" "$out"

# math builtins: sqrt, abs, min, max
$BIN -o build/math examples/math.amt
set +e
out=$(./build/math)
rc=$?
set -e
check "math output" "3
1.4142135623731
42
42
2.5
3
7
1.5
2.5
4.24264068711928
2" "$out"
check "math exit" "0" "$rc"

# string concatenation: + and +=
$BIN -o build/concat examples/concat.amt
set +e
out=$(./build/concat)
rc=$?
set -e
check "concat output" "amethyst is fun
Hello, world!
count: 123
amethyst
amethyst
amethyst!
1" "$out"
check "concat exit" "0" "$rc"

# dynamic arrays: [] + push/pop with automatic growth
$BIN -o build/dynarray examples/dynarray.amt
set +e
out=$(./build/dynarray)
rc=$?
set -e
check "dynarray output" "0
3
10
30
30
2
10
81
16
beta
1
3
7" "$out"
check "dynarray exit" "0" "$rc"

# methods: impl blocks, self and value.method(...)
$BIN -o build/methods examples/methods.amt
set +e
out=$(./build/methods)
rc=$?
set -e
check "methods output" "1
5
7
17
62
62
11
3" "$out"
check "methods exit" "0" "$rc"

# definite assignment: nested branches, shadowing, early return
$BIN -o build/definite examples/definite.amt
set +e
out=$(./build/definite)
rc=$?
set -e
check "definite-assignment output" "positive
non-positive
0
7
15
seen" "$out"

cat > build/da_nested.amt <<'EOF'
fn nested(c: bool) -> int {
    var y: int;
    if c {
        if c {
            y = 7;
        } else {
            y = 8;
        }
    } else {
        y = 9;
    }
    return y;
}

fn shadowed(c: bool) -> int {
    var x: int = 100;
    if c {
        var x: int;
        x = 5;
        print(x);
    }
    return x;
}

fn main() -> int {
    print(nested(true));
    print(nested(false));
    print(shadowed(true));
    print(shadowed(false));
    return 0;
}
EOF
$BIN -o build/da_nested build/da_nested.amt
set +e
out=$(./build/da_nested)
rc=$?
set -e
check "nested branch merge and shadowing" "7
9
5
100
100" "$out"

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

# structs on the heap: new / fields / null / free
$BIN -o build/structs examples/structs.amt
set +e
out=$(./build/structs)
rc=$?
set -e
expected_structs="10
2.5
centro
25
30
0
0
12
5
36
13
1
1"
check "structs output" "$expected_structs" "$out"
check "structs exit" "0" "$rc"

# heap arrays: new T[n] with a runtime length, zeroed, free()
$BIN -o build/heap examples/heap.amt
set +e
out=$(./build/heap)
rc=$?
set -e
expected_heap="8
0
49
140
2.5
0
0"
check "heap output" "$expected_heap" "$out"
check "heap exit" "0" "$rc"

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

fn strAndSlice(name: string, label: string, nums: int[]) -> int {
    if name == label {
        var total = 0;
        for i in 0..len(nums) {
            total += nums[i];
        }
        return total;
    }
    return len(name);
}

fn main() -> int {
    var s: int[3] = [10, 20, 30];
    print(seven(1, 2, 3, 4, 5, 6, 7));
    print(many(s, 1, 2, 3, 4, 5, 6));
    print(fourAndSlice(1, 2, 3, 4, s));
    print(fiveAndSlice(1, 2, 3, 4, 5, s));
    print(strAndSlice("same", "same", s));
    print(strAndSlice("ab", "cd", s));
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
75
60
2" "$out"

# float ABI: 8 SSE registers, then the stack; ints and floats classified apart
cat > build/fabi.amt <<'EOF'
fn ten(a: float, b: float, c: float, d: float, e: float,
       f: float, g: float, h: float, i: float, j: float) -> float {
    return a + b + c + d + e + f + g + h + i + j;
}

fn mixed(a: float, b: int, c: float, d: int, e: float, f: int,
         g: float, h: int, i: float, j: int) -> float {
    return a + float(b) + c + float(d) + e + float(f) + g + float(h)
           + i + float(j);
}

fn strs(s1: string, s2: string, x: float, n: int) -> float {
    if s1 == s2 {
        return x + float(n);
    }
    return x * float(n);
}

fn main() -> int {
    print(ten(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0));
    print(mixed(1.5, 10, 2.5, 20, 3.5, 30, 4.5, 40, 5.5, 50));
    print(strs("a", "a", 2.5, 3));
    print(strs("a", "b", 2.5, 3));
    return 0;
}
EOF
$BIN -o build/fabi build/fabi.amt
set +e
out=$(./build/fabi)
rc=$?
set -e
check "float/stack argument ABI" "55
167.5
5.5
7.5" "$out"

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

# runtime null reference (reading a field of a null object) → exit 1 + message
cat > build/nullref.amt <<'EOF'
struct Point { x: int }

fn main() -> int {
    var p: Point = null;
    print(p.x);
    return 0;
}
EOF
$BIN -o build/nullref build/nullref.amt
set +e
err=$(./build/nullref 2>&1)
rc=$?
set -e
if [[ $rc -eq 1 && "$err" == *"null reference"* ]]; then
  PASS=$((PASS + 1))
  echo "  PASS  runtime null reference"
else
  FAIL=$((FAIL + 1))
  echo "  FAIL  runtime null reference (rc=$rc)"
  echo "        err: $err"
fi

# `new T[n]` with a non-positive size → exit 1 + message
cat > build/badsize.amt <<'EOF'
fn main() -> int {
    var n = 0;
    var a = new int[n];
    print(len(a));
    return 0;
}
EOF
$BIN -o build/badsize build/badsize.amt
set +e
err=$(./build/badsize 2>&1)
rc=$?
set -e
if [[ $rc -eq 1 && "$err" == *"array size must be positive (got 0)"* ]]; then
  PASS=$((PASS + 1))
  echo "  PASS  runtime heap array size"
else
  FAIL=$((FAIL + 1))
  echo "  FAIL  runtime heap array size (rc=$rc)"
  echo "        err: $err"
fi

# pop() from an empty dynamic array → exit 1 + message
cat > build/popempty.amt <<'EOF'
fn main() -> int {
    var a: int[] = [];
    print(pop(a));
    return 0;
}
EOF
$BIN -o build/popempty build/popempty.amt
set +e
err=$(./build/popempty 2>&1)
rc=$?
set -e
if [[ $rc -eq 1 && "$err" == *"pop from an empty array"* ]]; then
  PASS=$((PASS + 1))
  echo "  PASS  runtime pop from empty array"
else
  FAIL=$((FAIL + 1))
  echo "  FAIL  runtime pop from empty array (rc=$rc)"
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
expect_fail "string into int" tests/err_string_var.amt "cannot initialize 'int n' with a string literal"
expect_fail "string arithmetic" tests/err_string_arith.amt "'+' expects two strings or two numbers, got 'string' and 'int'"
expect_fail "string compared with int" tests/err_string_cmp.amt "cannot compare 'string' with 'int' with == / !="
expect_fail "array compared" tests/err_array_cmp.amt "cannot compare arrays with == / !="
expect_fail "compound on bool" tests/err_compound_bool.amt "'+=' expects int on both sides"
expect_fail "len of non-array" tests/err_len_nonarray.amt "'len' expects an array or string argument"
expect_fail "len arity" tests/err_len_arity.amt "'len' expects 1 argument(s), got 0"
expect_fail "len redefinition" tests/err_len_redef.amt "'len' is a builtin and cannot be redefined"
expect_fail "array literal argument" tests/err_array_arg_literal.amt "must be a variable (assign the array first)"
expect_fail "slice element type" tests/err_slice_elem_type.amt "expected 'bool[]', got 'int[]'"
expect_fail "sized array parameter" tests/err_param_array_size.amt "array parameters must be written as a slice"
expect_fail "slice local variable" tests/err_slice_local.amt "local slices must be initialized with 'new'"
expect_fail "array copy" tests/err_array_copy.amt "must be initialized with an array literal"
expect_fail "float + int" tests/err_float_int_mix.amt "operator expects two floats, got 'float' and 'int'"
expect_fail "float modulo" tests/err_float_mod.amt "'%' has no float version"
expect_fail "float compared with int" tests/err_float_cmp_int.amt "cannot compare 'float' with 'int' with == / !="
expect_fail "int() of an int" tests/err_float_conv.amt "'int' expects a float argument, got 'int'"
expect_fail "read of unassigned var" tests/err_uninit_read.amt "variable 'x' is read before it is definitely assigned"
expect_fail "unassigned after if without else" tests/err_uninit_branch.amt "variable 'x' is read before it is definitely assigned"
expect_fail "unassigned after a loop" tests/err_uninit_loop.amt "variable 'x' is read before it is definitely assigned"
expect_fail "compound on unassigned var" tests/err_uninit_compound.amt "variable 'x' is read before it is definitely assigned"
expect_fail "var without a type" tests/err_uninit_infer.amt "needs an initializer when the type is omitted"
expect_fail "uninitialized array" tests/err_uninit_array.amt "array variables must be initialized"
expect_fail "null initializer" tests/err_struct_infer_null.amt "cannot infer type of 'q' from this initializer"
expect_fail "unknown struct field" tests/err_struct_field.amt "has no field 'xx' (did you mean 'x'?)"
expect_fail "missing struct field" tests/err_struct_missing.amt "missing field 'y' in the initializer of 'Point'"
expect_fail "duplicate struct field" tests/err_struct_dup.amt "field 'x' is initialized twice"
expect_fail "field not declared" tests/err_struct_nofield.amt "struct 'Point' has no field 'z'"
expect_fail "wrong field type" tests/err_struct_field_type.amt "field 'x' of 'Point': expected 'int', got 'string'"
expect_fail "print a struct" tests/err_struct_print.amt "print expects int, bool, float or string, got 'Point'"
expect_fail "compare structs" tests/err_struct_cmp.amt "cannot compare structs with == / !="
expect_fail "free an int" tests/err_free_nonstruct.amt "cannot free a value of type 'int'"
expect_fail "array struct field" tests/err_struct_array_field.amt "array fields are not supported yet"
expect_fail "unknown type" tests/err_unknown_type.amt "unknown type 'Foo'"
expect_fail "field on null literal" tests/err_null_field.amt "cannot read field 'x' from 'null'"
expect_fail "field of unassigned struct" tests/err_struct_uninit.amt "variable 'p' is read before it is definitely assigned"
expect_fail "struct array element type" tests/err_struct_elem_type.amt "array element 2 has type 'int', expected 'Point'"
expect_fail "null array element" tests/err_null_array.amt "array elements cannot be 'null'"
expect_fail "void array element" tests/err_void_array.amt "array elements must have a value"
expect_fail "new into fixed array" tests/err_heap_fixed.amt "cannot initialize the fixed-size 'int[3] a' with 'new'"
expect_fail "free a local array" tests/err_heap_free_local.amt "only an array variable initialized with 'new'"
expect_fail "free an array parameter" tests/err_heap_free_param.amt "only an array variable initialized with 'new'"
expect_fail "array size type" tests/err_heap_size_type.amt "array size must be int, got 'string'"
expect_fail "new as a statement" tests/err_heap_new_stmt.amt "arrays cannot be used in this context"
expect_fail "sqrt of an int" tests/err_sqrt_int.amt "'sqrt' expects a float argument, got 'int'"
expect_fail "min of mixed types" tests/err_min_mixed.amt "'min' expects two values of the same type"
expect_fail "abs of a string" tests/err_abs_type.amt "'abs' expects an int or float argument"
expect_fail "builtin redefinition" tests/err_builtin_redef.amt "'min' is a builtin and cannot be redefined"
expect_fail "sqrt arity" tests/err_sqrt_args.amt "'sqrt' expects 1 argument(s), got 2"
expect_fail "string += int" tests/err_concats_eq.amt "'+=' expects a string on both sides"
expect_fail "string -=" tests/err_string_subeq.amt "'-=' cannot be applied to strings"
expect_fail "push into fixed array" tests/err_push_fixed.amt "'push' expects a dynamic array, got fixed-size 'int[3]'"
expect_fail "push into a parameter" tests/err_push_param.amt "'push' cannot modify a slice parameter"
expect_fail "push of the wrong type" tests/err_push_type.amt "cannot push 'string' into 'int[]'"
expect_fail "push arity" tests/err_push_arity.amt "'push' expects 2 argument(s), got 1"
expect_fail "pop of a scalar" tests/err_pop_scalar.amt "'pop' expects an array as its first argument"
expect_fail "empty literal type inference" tests/err_empty_infer.amt "cannot infer the element type of an empty array literal"
expect_fail "method without self" tests/err_impl_self.amt "must take 'self: Point' as its first parameter"
expect_fail "method self type" tests/err_impl_self_type.amt "must take 'self: Point' as its first parameter"
expect_fail "duplicate method" tests/err_impl_dup_method.amt "duplicate method 'move' in impl 'Point'"
expect_fail "unknown method" tests/err_no_method.amt "type 'Point' has no method 'nope'"
expect_fail "method arity" tests/err_method_arity.amt "method 'Point.move' expects 1 argument(s), got 0"
expect_fail "method argument type" tests/err_method_arg_type.amt "argument 1 of 'Point.move': expected 'int', got 'string'"
expect_fail "assign to self" tests/err_self_assign.amt "cannot assign to 'self'"
expect_fail "reserved function name" tests/err_reserved_fn.amt "starting with '__amethyst_' are reserved"
expect_fail "method on a non-struct" tests/err_receiver_not_struct.amt "is a method call but the receiver is not a struct"
expect_fail "impl of an unknown struct" tests/err_impl_unknown.amt "impl for unknown struct 'Ghost'"

echo
echo "passed: $PASS  failed: $FAIL"
[[ $FAIL -eq 0 ]]
