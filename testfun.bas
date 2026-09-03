fun fib(n)
    if n <= 2
        return 1
    fi
    return fib(n-2)+fib(n-1)
endfun


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

endfun

print f(8)