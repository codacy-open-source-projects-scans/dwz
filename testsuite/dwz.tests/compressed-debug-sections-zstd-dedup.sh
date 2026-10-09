trap 'rm -f 1 dwz.err' EXIT

exec=$execs/min
cp $exec 1

if ! objcopy --compress-debug-sections=zstd 1 2>/dev/null; then
    exit 77
fi

if ! readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'; then
    exit 77
fi

if dwz 1 2>dwz.err; status=$?; then
    true
fi
if [ $status -ne 0 ] && grep -qi 'zstd\|unknown compression' dwz.err; then
    exit 77
fi
[ $status -eq 0 ]

readelf -S 1 \
	| grep -A1 '\.debug_' \
	| grep -v '\.debug_' \
	| grep -q 'C'

readelf -tW 1 | grep -A4 '\.debug_info$' | grep -q 'ZSTD'

readelf -wi 1 >/dev/null
