#!/usr/bin/env bas

fun maxArea(height, n)
    let left = 0
    let right = n - 1
    let best = 0

    do while left < right

        let width = right - left

        if height(left) < height(right)
            let shorter = height(left)
            let left = left + 1
        else
            let shorter = height(right)
            let right = right - 1
        fi

        let area = width * shorter
        if area > best
            let best = area
        fi

    done
    return best
end fun

let h1 = new(9)
h1(0) = 1
h1(1) = 8
h1(2) = 6
h1(3) = 2
h1(4) = 5
h1(5) = 4
h1(6) = 8
h1(7) = 3
h1(8) = 7
print "Example 1: ", maxArea(h1, 9), " (expected 49)\n"
free h1

let h2 = new(2)
h2(0) = 1
h2(1) = 1
print "Example 2: ", maxArea(h2, 2), " (expected 1)\n"
free h2

let h3 = new(5)
h3(0) = 4
h3(1) = 3
h3(2) = 2
h3(3) = 1
h3(4) = 4
print "Extra test: ", maxArea(h3, 5), " (expected 16)\n"
free h3

end
