#!/bin/sh
rc=0
for f in ./test-data/fonts/*; do
	rm -f ./tmpout.bmp
	if ! ./kfont-dump "$f" ./tmpout.bmp; then
		echo "--- FAILED to dump font file: $f" >&2
		echo >&2
		rc=1
		continue
	fi
	if ! cmp ./tmpout.bmp ./test-data/reference.bmp; then
		echo "--- MISMATCH output: $f" >&2
		backup=./tmpout_${f##*/}.bmp
		echo "    backup: $backup"
		cp ./tmpout.bmp "$backup"
		rc=1
		echo >&2
	fi
done
rm -f ./tmpout.bmp
exit $rc
