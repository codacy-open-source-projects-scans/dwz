if ! $execs/dwz-for-test --odr -v 2>/dev/null; then
    exit 77
fi

readelf_flags=""
if readelf -h 2>&1 | grep -q "\-wN"; then
    readelf_flags=-wN
fi

cp $execs/odr-struct 1
cp 1 2

for name in aaa bbb ccc; do
    cnt=$(readelf -wi 1 | grep -c "DW_AT_name.*:.*$name" || true)
    [ $cnt -eq 2 ]
done

for name in member_one member_two member_three member_four; do
    cnt=$(readelf -wi 1 | grep -c "DW_AT_name.*:.*$name" || true)
    case $name in
	member_one|member_two)
	    [ $cnt -eq 2 ]
	    ;;
	member_three|member_four)
	    [ $cnt -eq 1 ]
	    ;;
	esac
done

decl_cnt=$(readelf -wi 1 | grep -c "DW_AT_declaration" || true)

$execs/dwz-for-test --odr 1 2 -m 3

verify-dwarf.sh 1
verify-dwarf.sh 3

for name in aaa bbb ccc; do
    cnt=$(readelf -wi 3 | grep -c "DW_AT_name.*:.*$name" || true)
    [ $cnt -eq 1 ]
done

for name in member_one member_two member_three member_four; do
    cnt=$(readelf -wi 3 | grep -c "DW_AT_name.*:.*$name" || true)
    [ $cnt -eq 1 ]
done

# Even with -wN readelf 2.38-15.fc37 follows and prints the contents
# of the alt file. So make sure it cannot do that by removing it.
rm 3

for name in aaa bbb ccc; do
    cnt=$(readelf -wi $readelf_flags 1 | grep -c "DW_AT_name.*:.*$name" || true)
    [ $cnt -eq 0 ]
done

for name in member_one member_two member_three member_four; do
    cnt=$(readelf -wi $readelf_flags 1 | grep -c "DW_AT_name.*:.*$name" || true)
    [ $cnt -eq 0 ]
done

rm -f 1 2 3
