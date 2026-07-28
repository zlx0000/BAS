let i=0
dim notes1(8)
dim notes2(8)

notes1(0) = 81
notes1(1) = 106
notes1(2) = 125
notes1(3) = 54
notes1(4) = 106
notes1(5) = 81
notes1(6) = 54
notes1(7) = 37

notes2(0) = 66
notes2(1) = 89
notes2(2) = 125
notes2(3) = 54
notes2(4) = 89
notes2(5) = 66
notes2(6) = 54
notes2(7) = 37

1 i = i+1
let n = i / 16384

if (i / 65536)%4 = 0
    let notes = notes1
else
    let notes = notes2
fi

let q = (i * (notes(n % 8) + 51)) / 4096
let voice1 = 16 * (q%2)

let s = i / 131072

let a = n % 8
let b = (i / 8192) % 8

let t = (a % 2 + b % 2) % 2 + 2 * (((a / 2) % 2 + (b / 2) % 2) % 2) + 4 * (((a / 4) % 2 + (b / 4) % 2) % 2)

let x = s
q = (i * (notes(t) + 51)) / 1024
let voice2 = 16 * ((x % 2) * (q % 2) + 2 * ((x / 2) % 2) * ((q / 2) % 2))

x = s/3

t = (n + (i / 2048) % 3) % 8
q = (i * (notes(t) + 51)) / 1024
let voice3 = 16 * ((x % 2) * (q % 2) +2 * ((x / 2) % 2) * ((q / 2) % 2))
x = s / 5
t = (8 + n - (i / 1024) % 3) % 8
q = (i * (notes(t) + 51)) / 512
let voice4 = 16 * ((x % 2) * (q % 2) + 2 * ((x / 2) % 2) * ((q / 2) % 2))

putchar voice1 + voice2 + voice3 + voice4

goto 1
