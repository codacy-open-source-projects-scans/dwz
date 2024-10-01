if ! $execs/dwz-for-test --odr -v 2>/dev/null; then
    exit 77
fi

cp $execs/odr-struct-ns 1

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

# Check that bbb and ccc are present as DW_AT_declaration
readelf -wi 1 | grep -3 DW_AT_declaration > decls
grep bbb decls >/dev/null
grep ccc decls >/dev/null

$execs/dwz-for-test --odr 1

verify-dwarf.sh 1

for name in aaa bbb ccc; do
    cnt=$(readelf -wi 1 | grep -c "DW_AT_name.*:.*$name" || true)
    [ $cnt -eq 1 ]
done

for name in member_one member_two member_three member_four; do
    cnt=$(readelf -wi 1 | grep -c "DW_AT_name.*:.*$name" || true)
    [ $cnt -eq 1 ]
done

# We expect two decls to be removed, for bbb and ccc.
readelf -wi 1 | grep -3 DW_AT_declaration > decls || true

if grep bbb decls >/dev/null ; then exit 1; fi
if grep ccc decls >/dev/null ; then exit 2; fi

rm -f 1 decls
