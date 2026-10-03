#!/usr/bin/env bash
# Amethyst vs C / Python / Node benchmarks — fair comparison (C -O0, no closed-form).
# Writes build/bench/results.json for the website.
set -euo pipefail
export LC_ALL=C
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
mkdir -p build/bench/src

echo "=== Building Amethyst compiler if needed ==="
if [[ ! -x ./amethystc ]]; then make -s; fi

SRC="$ROOT/build/bench/src"
OUT="$ROOT/build/bench"

# ---------- sources ----------
# Workloads sized so process startup is not dominant; results verified equal.

# 1) fib recursive (pure call/return + branches)
cat > "$SRC/fib.amt" <<'EOF'
fn fib(n: int) -> int {
    if n < 2 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fn main() -> int {
    print(fib(35));
    return 0;
}
EOF
cat > "$SRC/fib.c" <<'EOF'
#include <stdio.h>
static long fib(long n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
int main(void) { printf("%ld\n", fib(35)); return 0; }
EOF
cat > "$SRC/fib.py" <<'EOF'
def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)
print(fib(35))
EOF
cat > "$SRC/fib.js" <<'EOF'
function fib(n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
console.log(fib(35));
EOF

# 2) hot integer loop — result stays within JS 2^53 safe-integer range
cat > "$SRC/loop.amt" <<'EOF'
fn main() -> int {
    var i: int = 0;
    var sum: int = 0;
    while i < 300000000 {
        sum = sum + (i % 7) * (i % 11) + (i % 13);
        i = i + 1;
    }
    print(sum);
    return 0;
}
EOF
cat > "$SRC/loop.c" <<'EOF'
#include <stdio.h>
int main(void) {
    long i = 0, sum = 0;
    for (i = 0; i < 300000000L; i++) sum += (i % 7) * (i % 11) + (i % 13);
    printf("%ld\n", sum);
    return 0;
}
EOF
cat > "$SRC/loop.py" <<'EOF'
s = 0
for i in range(300000000):
    s += (i % 7) * (i % 11) + (i % 13)
print(s)
EOF
cat > "$SRC/loop.js" <<'EOF'
let s = 0;
for (let i = 0; i < 300000000; i++) s += (i % 7) * (i % 11) + (i % 13);
console.log(s);
EOF

# 3) nested loops with mixed ops
cat > "$SRC/nested.amt" <<'EOF'
fn main() -> int {
    var total: int = 0;
    var i: int = 0;
    while i < 4500 {
        var j: int = 0;
        while j < 4500 {
            total = total + (i * j + i) % 7;
            j = j + 1;
        }
        i = i + 1;
    }
    print(total);
    return 0;
}
EOF
cat > "$SRC/nested.c" <<'EOF'
#include <stdio.h>
int main(void) {
    long total = 0;
    for (long i = 0; i < 4500; i++)
        for (long j = 0; j < 4500; j++)
            total += (i * j + i) % 7;
    printf("%ld\n", total);
    return 0;
}
EOF
cat > "$SRC/nested.py" <<'EOF'
total = 0
for i in range(4500):
    for j in range(4500):
        total += (i * j + i) % 7
print(total)
EOF
cat > "$SRC/nested.js" <<'EOF'
let total = 0;
for (let i = 0; i < 4500; i++)
  for (let j = 0; j < 4500; j++)
    total += (i * j + i) % 7;
console.log(total);
EOF

# 4) prime count (trial division) — many calls + branches
cat > "$SRC/prime.amt" <<'EOF'
fn isPrime(n: int) -> bool {
    if n < 2 {
        return false;
    }
    var d: int = 2;
    while d * d <= n {
        if n % d == 0 {
            return false;
        }
        d = d + 1;
    }
    return true;
}

fn main() -> int {
    var count: int = 0;
    var n: int = 2;
    while n < 50000 {
        if isPrime(n) {
            count = count + 1;
        }
        n = n + 1;
    }
    print(count);
    return 0;
}
EOF
cat > "$SRC/prime.c" <<'EOF'
#include <stdio.h>
#include <stdbool.h>
static bool isPrime(long n) {
    if (n < 2) return false;
    for (long d = 2; d * d <= n; d++)
        if (n % d == 0) return false;
    return true;
}
int main(void) {
    long count = 0;
    for (long n = 2; n < 50000; n++)
        if (isPrime(n)) count++;
    printf("%ld\n", count);
    return 0;
}
EOF
cat > "$SRC/prime.py" <<'EOF'
def is_prime(n):
    if n < 2:
        return False
    d = 2
    while d * d <= n:
        if n % d == 0:
            return False
        d += 1
    return True

count = 0
for n in range(2, 50000):
    if is_prime(n):
        count += 1
print(count)
EOF
cat > "$SRC/prime.js" <<'EOF'
function isPrime(n) {
  if (n < 2) return false;
  for (let d = 2; d * d <= n; d++)
    if (n % d === 0) return false;
  return true;
}
let count = 0;
for (let n = 2; n < 50000; n++)
  if (isPrime(n)) count++;
console.log(count);
EOF

echo "=== Compiling ==="
for b in fib loop nested prime; do
  ./amethystc -o "$OUT/amethyst_$b" "$SRC/$b.amt"
  gcc -O0 -o "$OUT/c_$b" "$SRC/$b.c"   # fair: no optimizer (Amethyst has none either)
done
echo "done"

# ---------- correctness ----------
echo "=== Correctness ==="
for b in fib loop nested prime; do
  a=$("$OUT/amethyst_$b")
  c=$("$OUT/c_$b")
  p=$(python3 "$SRC/$b.py")
  j=$(node "$SRC/$b.js")
  echo "$b: A=$a C=$c Py=$p Js=$j"
  [[ "$a" == "$c" && "$c" == "$p" && "$p" == "$j" ]] || { echo "MISMATCH $b"; exit 1; }
done

# ---------- timing ----------
best_of() {
  local n="$1"; shift
  local best=""
  for _ in $(seq 1 "$n"); do
    local t0 t1 dt
    t0=$(date +%s.%N)
    "$@" >/dev/null
    t1=$(date +%s.%N)
    dt=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.4f", b-a}')
    if [[ -z "$best" ]] || awk -v a="$dt" -v b="$best" 'BEGIN{exit !(a<b)}'; then
      best="$dt"
    fi
  done
  echo "$best"
}

echo "=== Timing (best of N) ==="
# JSON assembly
json_tmp="$OUT/results.json.tmp"
printf '{\n  "meta": {\n    "date": "%s",\n    "note": "C compiled with gcc -O0 (fair: Amethyst codegen is unoptimized). Best of N runs.",\n    "machine": "%s"\n  },\n  "benchmarks": [\n' \
  "$(date -u +%Y-%m-%d)" "$(uname -m)-$(uname -s)" > "$json_tmp"

bench_meta=(
  "fib|Recursive fibonacci|fib(35)|"
  "loop|Hot integer loop|3e8 iterations with % and *|"
  "nested|Nested loops|4500×4500 iterations|"
  "prime|Prime counting|trial division up to 50 000|"
)

first=1
for meta in "${bench_meta[@]}"; do
  IFS='|' read -r key name workload _ <<< "$meta"
  runs=3
  # heavy ones: fewer runs already 3; python loop/nested slow -> still 3 ok
  
  line_parts=()
  for lang in amethyst c python javascript; do
    case "$lang" in
      amethyst) cmd="$OUT/amethyst_$key"; label="Amethyst" ;;
      c)        cmd="$OUT/c_$key"; label="C" ;;
      python)   cmd="python3 $SRC/$key.py"; label="Python" ;;
      javascript) cmd="node $SRC/$key.js"; label="JavaScript" ;;
    esac
    # skip if python would take forever — still fine at these sizes
    t=$(best_of "$runs" bash -c "$cmd")
    echo "$key $label ${t}s"
    json_key="$lang"; [[ "$lang" == "node" ]] && json_key="javascript"; line_parts+=("\"$json_key\": $t")
  done

  # relative to C = 1.0
  # we'll compute client-side from the raw seconds

  if [[ $first -eq 0 ]]; then printf ',\n' >> "$json_tmp"; fi
  first=0
  printf '    {\n      "id": "%s",\n      "name": "%s",\n      "workload": "%s",\n      "times": { %s }\n    }' \
    "$key" "$name" "$workload" "$(IFS=', '; echo "${line_parts[*]}")" >> "$json_tmp"
done

printf '\n  ]\n}\n' >> "$json_tmp"
mv "$json_tmp" "$OUT/results.json"
echo "=== wrote $OUT/results.json ==="
cat "$OUT/results.json"
