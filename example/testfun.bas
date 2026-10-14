#!/usr/bin/env bas

let cnt = 0

fun fib(n)
    cnt = cnt + 1
    if n <= 2
        return 1
    fi
    return fib(n-2)+fib(n-1)
end fun

fun local_alias_test
    let alias = &fib
    let result = alias(10)
    let alias = 0
    return result = 55 and alias = 0
end fun

fun apply(fn)
    return fn(10)
end fun

fun return_fib
    return &fib
end fun

fun return_local_alias
    let alias = &fib
    return &alias
end fun

fun return_function_parameter(fn)
    return &fn
end fun

# Function alias regression checks.
let result = fib(10)
if result != 55 or cnt != 109
    print "FAIL: recursive function call\n"
    end
fi

let fib_alias = &fib
let result = fib_alias(10)
if result != 55
    print "FAIL: function alias call\n"
    end
fi

let alias_copy = &fib_alias
let result = alias_copy(10)
let alias_copy = 0
if result != 55 or alias_copy != 0
    print "FAIL: alias of alias\n"
    end
fi

let fib_alias = 0
let result = fib(10)
if fib_alias != 0 or result != 55
    print "FAIL: alias reassignment changed the original function\n"
    end
fi

let result = local_alias_test
if not result
    print "FAIL: local alias reassignment\n"
    end
fi

let result = apply(&fib)
if result != 55
    print "FAIL: function parameter\n"
    end
fi
print "PASS: function aliases\n"

# Returned functions must remain callable after their call frame is gone.
let returned_alias = return_fib
let result = returned_alias(10)
let returned_alias = 0
if result != 55 or returned_alias != 0
    print "FAIL: returning a global function\n"
    end
fi

let returned_alias = return_local_alias
let result = returned_alias(10)
let returned_alias = 0
if result != 55 or returned_alias != 0
    print "FAIL: returning a local function alias\n"
    end
fi

let returned_alias = return_function_parameter(return_local_alias)
let result = returned_alias(10)
let returned_alias = 0
if result != 55 or returned_alias != 0
    print "FAIL: forwarding a returned function through a parameter\n"
    end
fi

let result = apply(return_fib)
if result != 55
    print "FAIL: calling a returned function through a parameter\n"
    end
fi

let result = fib(10)
if result != 55
    print "FAIL: returned alias reassignment changed the original function\n"
    end
fi
print "PASS: returning functions\n"

# Run these expected-error checks in the REPL after defining fib above.
# File execution stops at a runtime error, so these are manual checks.
# fib = 0              -> can't overwrite a function
# print fib(10)        -> 55
# let fib_alias = fib  -> param size doesn't match
# let fib_alias = &fib -> succeeds after the failed call
# print fib_alias(10)  -> 55
# let fib_alias = 0    -> succeeds
# print fib_alias      -> 0


fun f(x)
    PRINT "TEST GOTO IN IF\n"
    GOTO 3

    LET A=1

    IF A = 0
        PRINT "TEST FAILD\n"
        IF A=1
            PRINT "TEST FAILD\n"
1           PRINT "TEST "
            GOTO 2
        ELSE
            PRINT "TEST FAILD\n"
            IF A=0
2               PRINT "PASSED\n"
                goto 10
            ELSE
3               LET B=0
                GOTO 5
                PRINT "TEST FAILD\n"
            FI
        FI
    ELSE
5       IF B=0 THEN 1
    FI


10  let n = fib(x)
    let val = 0
    let nxt = 1
    let in = new(2)
    let cur = in

    let head = new(2)
    let cur2 = head
    for i = 1 to 1000
        cur2(1) = new(2)
        cur2 = cur2(1)
    next i

    cur2 = head

    for i = n to 1 step -1
        cur(val) = i
        if i != 1
            cur(nxt) = new(2)
        fi
        cur = cur(nxt)
    next i

    let stack = new(n)
    let top = 0

    cur = in
11  stack(top)=cur
    top = top + 1
    cur = cur(nxt)
    if cur != 0 then 11


    print in
    print stack

    let d = new(2)
    cur = d

    for i = n-1 to 0 step -1
        cur(nxt) = stack(i)
        cur = cur(nxt)
    next i
    cur(nxt) = 0

    free stack

    return d(nxt)

end fun

fun n(x)
    return x
end fun

fun take(a,b,c)
    a(0) = b + c
    return new(a(0))
end fun

fun id(a)
    return a
end fun

fun collect
    gc
    return 0
end fun

1 print "num = "
    let num = getint
    if num <= 0 or num > 50
        end
    fi
    let ret = f(num)
    print ret, "\n"

    free ret

    let ret = take(id(new(1)),id(id(1)), collect)
    print ret, "\n"
    free ret
    let a = id(new(num))
    for i = 0 to num - 1
        a(i) = i + 1
    next i

    print a, "\n"
    free a
goto 1
