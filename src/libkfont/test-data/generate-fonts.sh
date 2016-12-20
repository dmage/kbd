#!/bin/sh

u32le() {
    perl -e 'print pack("L",$ARGV[0])' "$1"
}

BITMAP=./bitmap.raw
HEIGHT=16

psf1() {
    printf '\x36\x04' # magic
    printf '\x00' # mode
    u32le $HEIGHT # char size
    cat "$1"
}

psf2() {
    printf '\x72\xb5\x4a\x86' # magic
    printf '\x00\x00\x00\x00' # version
    printf '\x20\x00\x00\x00' # header size
    printf '\x00\x00\x00\x00' # flags
    printf '\x00\x01\x00\x00' # length
    u32le $HEIGHT # char size
    u32le $HEIGHT # height
    printf '\x08\x00\x00\x00' # width
    cat "$1"
}

part() {
    printf '\x72\xb5\x4a\x86' # magic
    printf '\x00\x00\x00\x00' # version
    printf '\x20\x00\x00\x00' # header size
    printf '\x00\x00\x00\x00' # flags
    u32le $(($3 - $2)) # length
    u32le $HEIGHT # char size
    u32le $HEIGHT # height
    printf '\x08\x00\x00\x00' # width
    dd if="$1" bs=9 skip="$2" count=$(($3 - $2)) 2>/dev/null
}

mkdir -p ./fonts
psf1 ./bitmap.raw >./fonts/psf1.psf
cat ./fonts/psf1.psf | gzip > ./fonts/psf1.psf.gz
psf2 ./bitmap.raw >./fonts/psf2.psf
cat ./fonts/psf2.psf | gzip > ./fonts/psf2.psf.gz
cp ./bitmap.raw ./fonts/legacy.fnt
(head -c40 /dev/zero; cat ./bitmap.raw) >./fonts/legacy40.fnt
(head -c40 /dev/zero; cat ./bitmap.raw; head -c$((6 + 14*256 + 6 + 8*256)) /dev/zero) >./fonts/legacy9780.cp
# TODO: 32768

mkdir -p ./parts
part ./bitmap.raw 0 32 >./parts/part0
part ./bitmap.raw 32 128 >./parts/part1
part ./bitmap.raw 128 256 | gzip >./parts/part2.gz
cat <<END >./fonts/partial.$HEIGHT
# combine partial fonts
part0
part1
part2
END
