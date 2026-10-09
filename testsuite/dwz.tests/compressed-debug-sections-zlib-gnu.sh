trap 'rm -f 1' EXIT

exec=$execs/hello
cp $exec 1

# Skip if objcopy does not support the legacy GNU compression.
if ! objcopy --compress-debug-sections=zlib-gnu 1 2>/dev/null; then
    exit 77
fi

# zlib-gnu renames .debug_* to .zdebug_*; skip if that did not happen.
if ! readelf -S 1 | grep -q '\.zdebug_'; then
    exit 77
fi

dwz 1

# Section names must still be .zdebug_* (no SHF_COMPRESSED on these).
readelf -S 1 | grep -q '\.zdebug_info'

# DWARF must still be readable.
readelf -wi 1 >/dev/null
