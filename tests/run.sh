#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/jankyborders-tests.XXXXXX")"
# Leave failures inspectable; successful binaries are temporary and contain no user data.
flags="-std=c99 -O1 -g -fsanitize=address -fno-omit-frame-pointer"
clang $flags ${TEST_CFLAGS:-} tests/selection.c src/hashtable.c -o "$build_dir/selection" -framework AppKit -framework CoreVideo -framework QuartzCore -F/System/Library/PrivateFrameworks -framework SkyLight
"$build_dir/selection"
clang $flags -DTEST_TIMERS tests/selection.c src/hashtable.c -o "$build_dir/timers" -framework AppKit -framework CoreVideo -framework QuartzCore -F/System/Library/PrivateFrameworks -framework SkyLight
"$build_dir/timers"
clang $flags tests/renderer.m -o "$build_dir/renderer" -framework AppKit -framework CoreVideo -framework QuartzCore -F/System/Library/PrivateFrameworks -framework SkyLight
"$build_dir/renderer"
clang $flags tests/border_lifetime.c src/animation.c src/layer.m -o "$build_dir/lifetime" -framework AppKit -framework CoreVideo -framework QuartzCore -F/System/Library/PrivateFrameworks -framework SkyLight
"$build_dir/lifetime" move
"$build_dir/lifetime"
