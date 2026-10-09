trap 'rm -f 1' EXIT

exec=$execs/min
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

# A multi-CU binary exercises the compressed-output path in write_dso
# where dwz has rewritten .debug_info (new_data != NULL) and the
# malloced buffer is what gets recompressed, not the libelf-owned input.
dwz 1

readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'

readelf -tW 1 | grep -A4 '\.debug_info$' | grep -q 'ZLIB'

readelf -wi 1 >/dev/null
