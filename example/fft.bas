#!/usr/bin/env bas

# In-place radix-2 FFT. size must be a positive power of two.
# Set inverse to 1 for a normalized inverse transform, or 0 for a forward FFT.
fun fft(ar, ai, size, inverse)
    let pi = 3.141592653589793
    let direction = -1.0
    if inverse
        direction = 1.0
    fi

    # Put the input in bit-reversed order.
    let j = 0
    for i = 1 to size - 1
        let bit = size / 2
        do while j >= bit
            j = j - bit
            bit = bit / 2
        done
        j = j + bit

        if i < j
            let tmp = ar(i)
            ar(i) = ar(j)
            ar(j) = tmp

            tmp = ai(i)
            ai(i) = ai(j)
            ai(j) = tmp
        fi
    next i

    # Combine successively larger groups of samples.
    let m = 2
    do while m <= size
        let half = m / 2
        let angle = direction * 2.0 * pi / m
        let wmre = cos(angle)
        let wmim = sin(angle)

        for j = 0 to size - 1 step m
            let wre = 1.0
            let wim = 0.0

            for k = 0 to half - 1
                let apos = j + k
                let bpos = apos + half
                let tre = wre * ar(bpos) - wim * ai(bpos)
                let tim = wre * ai(bpos) + wim * ar(bpos)
                let ure = ar(apos)
                let uim = ai(apos)

                ar(apos) = ure + tre
                ai(apos) = uim + tim
                ar(bpos) = ure - tre
                ai(bpos) = uim - tim

                let nwre = wre * wmre - wim * wmim
                wim = wre * wmim + wim * wmre
                wre = nwre
            next k
        next j

        m = m * 2
    done

    if inverse
        let inv_size = 1.0 / size
        for i = 0 to size - 1
            ar(i) = ar(i) * inv_size
            ai(i) = ai(i) * inv_size
        next i
    fi
    return 0
end fun

fun print_samples(ar, ai, count)
    for i = 0 to count - 1
        print i, " ", ar(i), " ", ai(i), "\n"
    next i
    return 0
end fun

let pi = 3.141592653589793
let n = 15
let size = 1
for i = 1 to n
    size = size * 2
next i

let ar = new(size)
let ai = new(size)
let freq = 5
for i = 0 to size - 1
    ar(i) = sin(2.0 * pi * freq * i / size)
    ai(i) = 0.0
next i

let printsize = size
if printsize > 32
    printsize = 32
fi

print "INPUT\n"
let _ = print_samples(ar, ai, printsize)

let _ = fft(ar, ai, size, 0)
print "FFT\n"
let _ = print_samples(ar, ai, printsize)

let _ = fft(ar, ai, size, 1)
print "IFFT\n"
let _ = print_samples(ar, ai, printsize)

free ar
free ai
