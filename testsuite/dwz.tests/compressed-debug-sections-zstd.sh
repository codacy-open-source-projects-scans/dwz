trap 'rm -f 1 dwz.err' EXIT

exec=$execs/hello
cp $exec 1

# Skip if objcopy does not support zstd compression.
if ! objcopy --compress-debug-sections=zstd 1 2>/dev/null; then
    exit 77
fi

if ! readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'; then
    exit 77
fi

# Skip if libelf cannot decompress / recompress zstd: older libelf
# without --enable-zstd reports "unknown compression type".
if dwz 1 2>dwz.err; status=$?; then
    true
fi
if [ $status -ne 0 ] && grep -qi 'zstd\|unknown compression' dwz.err; then
    exit 77
fi
[ $status -eq 0 ]

# The compressed flag must still be set on .debug_* sections.
readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'

# Compression type must still be ZSTD.
readelf -tW 1 | grep -A4 '\.debug_info$' | grep -q 'ZSTD'

# DWARF must still be readable.
readelf -wi 1 >/dev/null
