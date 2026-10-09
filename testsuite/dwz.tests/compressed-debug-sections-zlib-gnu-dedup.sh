trap 'rm -f 1' EXIT

exec=$execs/min
cp $exec 1

if ! objcopy --compress-debug-sections=zlib-gnu 1 2>/dev/null; then
    exit 77
fi

if ! readelf -S 1 | grep -q '\.zdebug_'; then
    exit 77
fi

dwz 1

readelf -S 1 | grep -q '\.zdebug_info'

readelf -wi 1 >/dev/null
