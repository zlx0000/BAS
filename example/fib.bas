#!/usr/bin/env bas

fun fib(n)
    cnt = cnt + 1
    if n <= 2
        return 1
    fi
    return fib(n-2)+fib(n-1)
end fun

do while true
    let cnt = 0
    print "n = "
    let n = getint
    if n <= 0
        end
    fi
    let a = new(n)
    for i = 1 to n
    a(i-1)=fib(i)
    next i
    print a
    free a
    print "fib() is ran ", cnt, " times\n"
done
