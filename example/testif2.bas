#!/usr/bin/env bas

# An elif chain executes only the first matching branch.
# Exercise the first, middle, last and fallback branches.
for a = 0 to 6
    print "a = ", a, ": "
    if a <= 1
        print "at most one\n"
    elif a <= 2
        print "two\n"
    elif a <= 3
        print "three\n"
    elif a <= 4
        print "four\n"
    elif a <= 5
        print "five\n"
    else
        print "above five\n"
    fi
next a
