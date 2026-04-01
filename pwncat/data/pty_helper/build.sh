#!/bin/sh
# Build static pty_helper binaries for all supported architectures.
# Requires Docker.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$SCRIPT_DIR/pty_helper.c"
OUT="$SCRIPT_DIR"

ARCHS="x86_64 aarch64 i686 armv7l"

for arch in $ARCHS; do
    case "$arch" in
        x86_64)  image="alpine" ; platform="linux/amd64"  ;;
        aarch64) image="alpine" ; platform="linux/arm64"   ;;
        i686)    image="i386/alpine" ; platform="linux/386" ;;
        armv7l)  image="arm32v7/alpine" ; platform="linux/arm/v7" ;;
    esac

    echo "Building pty_helper for $arch..."
    docker run --rm --platform "$platform" \
        -v "$SCRIPT_DIR:/src" \
        "$image" \
        sh -c "apk add --no-cache gcc musl-dev linux-headers >/dev/null 2>&1 && \
               gcc -static -Os -s -o /src/pty_helper_$arch /src/pty_helper.c -lutil && \
               echo 'OK: pty_helper_$arch ($(wc -c < /src/pty_helper_$arch) bytes)'"
done

echo "Done. Built binaries:"
ls -la "$OUT"/pty_helper_*
