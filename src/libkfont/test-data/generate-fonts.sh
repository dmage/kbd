#!/bin/sh

u8() {
    local x
    for x do
        eval "printf '\\$(printf "%03o" $((x)))'"
    done
}

u32le() {
    local x=$1
    for i in 1 2 3 4; do
        u8 $((x & 0xff))
        x=$((x >> 8))
    done
}

BITMAP=./bitmap.raw
HEIGHT=16

psf1() {
    u8 0x36 0x04 # magic
    u8 0x00 # mode
    u8 $HEIGHT # char size
    cat "$1"
}

psf2() {
    u8 0x72 0xb5 0x4a 0x86 # magic
    u32le 0 # version
    u32le 0x20 # header size
    u32le 0 # flags
    u32le 256 # length
    u32le $HEIGHT # char size
    u32le $HEIGHT # height
    u32le 8 # width
    cat "$1"
}

part() {
    u8 0x72 0xb5 0x4a 0x86 # magic
    u32le 0 # version
    u32le 0x20 # header size
    u32le 0 # flags
    u32le $(($3 - $2)) # length
    u32le $HEIGHT # char size
    u32le $HEIGHT # height
    u32le 8 # width
    dd if="$1" bs=$HEIGHT skip="$2" count=$(($3 - $2)) 2>/dev/null
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
