#!/usr/bin/env bash
# Build Amethyst Dev Kit .deb — everything a beginner needs, no manual compile.
set -euo pipefail
export LC_ALL=C
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PKG_NAME="amethyst"
PKG_VERSION="${AMETHYST_VERSION:-1.0.0}"
PKG_ARCH="amd64"
PKG_MAINTAINER="Amethyst Project <amethyst@localhost>"
PKG_SECTION="devel"
PKG_PRIORITY="optional"
DEB_FILE="${PKG_NAME}-devkit_${PKG_VERSION}_${PKG_ARCH}.deb"
STAGE="build/deb"

echo "=== Amethyst Dev Kit packaging ==="
echo "version: $PKG_VERSION  arch: $PKG_ARCH"

# 1) Build release compiler
echo "=== Building amethystc ==="
make -s clean
make -s -j"$(nproc)"

if [[ ! -x ./amethystc ]]; then
  echo "error: amethystc missing after build" >&2
  exit 1
fi

# 2) Smoke-test the compiler before packaging
echo "=== Smoke test ==="
mkdir -p build/deb-smoke
cat > build/deb-smoke/hello.amt <<'EOF'
fn main() -> int {
    print(42);
    return 0;
}
EOF
./amethystc -o build/deb-smoke/hello build/deb-smoke/hello.amt
out=$(./build/deb-smoke/hello)
[[ "$out" == "42" ]] || { echo "smoke test failed: got '$out'"; exit 1; }
echo "ok: hello -> $out"

# 3) Stage tree
echo "=== Staging $STAGE ==="
rm -rf "$STAGE"
BIN_DIR="$STAGE/usr/bin"
DOC_DIR="$STAGE/usr/share/doc/$PKG_NAME"
EX_DIR="$STAGE/usr/share/$PKG_NAME/examples"
TPL_DIR="$STAGE/usr/share/$PKG_NAME/templates"
MAN_DIR="$STAGE/usr/share/man/man1"

mkdir -p "$BIN_DIR" "$DOC_DIR" "$EX_DIR" "$TPL_DIR" "$MAN_DIR" "$STAGE/DEBIAN"

# compiler (strip for size)
install -m 755 amethystc "$BIN_DIR/amethystc"
strip --strip-unneeded "$BIN_DIR/amethystc" 2>/dev/null || true

# helper: amethyst-new — create a starter file for beginners
install -m 755 packaging/deb/amethyst-new "$BIN_DIR/amethyst-new"

# examples
install -m 644 examples/*.amt "$EX_DIR/"

# templates
install -m 644 packaging/deb/templates/hello.amt "$TPL_DIR/hello.amt"
install -m 644 packaging/deb/templates/fib.amt "$TPL_DIR/fib.amt"

# docs
install -m 644 README.md "$DOC_DIR/"
install -m 644 packaging/deb/copyright "$DOC_DIR/copyright"
install -m 644 packaging/deb/changelog "$DOC_DIR/changelog.Debian"
gzip -9n -f "$DOC_DIR/changelog.Debian"

# man page
install -m 644 packaging/deb/amethystc.1 "$MAN_DIR/amethystc.1"
gzip -9n -f "$MAN_DIR/amethystc.1"
install -m 644 packaging/deb/amethyst-new.1 "$MAN_DIR/amethyst-new.1"
gzip -9n -f "$MAN_DIR/amethyst-new.1"

# 4) control + scripts
BIN_SIZE=$(du -sk "$STAGE" | cut -f1)
cat > "$STAGE/DEBIAN/control" <<EOF
Package: $PKG_NAME
Version: $PKG_VERSION
Section: $PKG_SECTION
Priority: $PKG_PRIORITY
Architecture: $PKG_ARCH
Maintainer: $PKG_MAINTAINER
Installed-Size: $BIN_SIZE
Depends: gcc, binutils
Recommends: make
Homepage: https://github.com/amethyst-lang/amethyst
Description: Amethyst language Dev Kit (compiler + examples + docs)
 Amethyst is a small compiled language with syntax halfway between
 high-level (Python, JS) and low-level (C) languages. Fast, statically
 typed, minimally readable.
 .
 This Dev Kit installs everything needed to write and run Amethyst
 programs without building the compiler by hand:
 .
  - amethystc     ahead-of-time compiler (C++17, x86-64 GAS backend)
  - amethyst-new  creates a ready-to-run starter project
  - examples and templates
  - README, man pages
 .
 The compiler invokes the system assembler (as) and linker (gcc/ld)
 at compile time, hence the Depends on gcc and binutils.
EOF

install -m 755 packaging/deb/postinst "$STAGE/DEBIAN/postinst"
install -m 755 packaging/deb/postrm "$STAGE/DEBIAN/postrm"

# 5) build .deb
OUT_DIR="dist"
mkdir -p "$OUT_DIR"
OUT_PATH="$OUT_DIR/$DEB_FILE"

echo "=== Building $OUT_PATH ==="
fakeroot dpkg-deb --build --root-owner-group "$STAGE" "$OUT_PATH"

echo "=== Package info ==="
dpkg-deb --info "$OUT_PATH"
echo
echo "=== Contents (top) ==="
dpkg-deb --contents "$OUT_PATH" | head -40
echo "..."
echo
echo "=== Install with ==="
echo "  sudo apt install ./$OUT_PATH"
echo "  # or: sudo dpkg -i $OUT_PATH && sudo apt-get install -f"
echo
echo "done: $OUT_PATH ($(du -h "$OUT_PATH" | cut -f1))"
