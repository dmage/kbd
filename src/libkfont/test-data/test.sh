#!/bin/sh
for f in ./fonts/*; do
	rm -f ./tmpout.bmp
	if ! ../kfont-dump "$f" tmpout.bmp; then
		echo "--- FAILED to dump font file: $f" >&2
		echo >&2
		continue
	fi
	if ! cmp tmpout.bmp reference.bmp; then
		echo "--- MISMATCH output: $f" >&2
		echo >&2
	fi
done
rm -f ./tmpout.bmp
