trap 'rm -f 1' EXIT

exec=$execs/hello
cp $exec 1
objcopy --compress-debug-sections=zlib 1
if readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'; then
    true
else
    exit 77
fi

dwz 1

# The compressed flag must still be set on .debug_* sections.
readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'

# Compression type must still be ZLIB.
readelf -tW 1 | grep -A4 '\.debug_info$' | grep -q 'ZLIB'

# DWARF must still be readable.
readelf -wi 1 >/dev/null
