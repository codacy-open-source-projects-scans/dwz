trap 'rm -f 1 2 3' EXIT

cp $execs/hello 1
cp $execs/hello 2
objcopy --compress-debug-sections=zlib 1
objcopy --compress-debug-sections=zlib 2
if readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'; then
    true
else
    exit 77
fi

dwz -m 3 1 2

# Each per-DSO output must still be compressed in the input scheme.
readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'
readelf -tW 1 | grep -A4 '\.debug_info$' | grep -q 'ZLIB'
readelf -wi 1 >/dev/null

readelf -S 2 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'
readelf -tW 2 | grep -A4 '\.debug_info$' | grep -q 'ZLIB'

# The shared .dwz file is intentionally emitted uncompressed; verify it
# exists, is uncompressed, and parses as DWARF.
[ -f 3 ]
if readelf -S 3 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'; then
    echo "shared .dwz file should be uncompressed" >&2
    exit 1
fi
readelf -wi 3 >/dev/null

[ "$(gnu-debugaltlink-name.sh 1)" = "3" ]
[ "$(gnu-debugaltlink-name.sh 2)" = "3" ]
