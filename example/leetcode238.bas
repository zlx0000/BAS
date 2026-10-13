#!/usr/bin/env bas

fun productExceptSelf(nums, n)
    let answer = new(n)
    let prefix = 1

    for i = 0 to n - 1
        answer(i) = prefix
        prefix = prefix * nums(i)
    next i

    let suffix = 1

    for i = n - 1 to 0 step -1
        answer(i) = answer(i) * suffix
        suffix = suffix * nums(i)
    next i

    return answer
end fun

let nums1 = new(4)
nums1(0) = 1
nums1(1) = 2
nums1(2) = 3
nums1(3) = 4
let answer1 = productExceptSelf(nums1, 4)
print "Example 1: ", answer1, " (expected [24,12,8,6])\n"
print "Input after call: ", nums1, "\n"
free nums1
free answer1

let nums2 = new(5)
nums2(0) = -1
nums2(1) = 1
nums2(2) = 0
nums2(3) = -3
nums2(4) = 3
let answer2 = productExceptSelf(nums2, 5)
print "Example 2: ", answer2, " (expected [0,0,9,0,0])\n"
free nums2
free answer2

end
