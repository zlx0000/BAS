#!/usr/bin/env bas

fun f(x)
let n = 1
let m = 1
if 0
    do while n/0 < x
        print n
        n = n + 1
    done
fi
do while n < x
    m = 1
    do while m < x
        print n, ",", m, "\n"
        if 1
            print "Y"
            goto 2
        else
            print "N\n"
2           print "Y\n"
        fi
        m = m + 1
    done
    n = n + 1
done
end fun

let _ = f(10)

let x = 3
let n = 1
let m = 1
if 0
    do while n/0 < x
        print n
        n = n + 1
    done
fi
do while n < x
    m = 1
    do while m < x
        print n, ",", m, "\n"
        for i = 1 to 3
            print i
            if i != 3
                print ","
            fi
        next i
        print "\n"
        if 1
            print "Y,"
            goto 2
        else
            print "N\n"
2           print "Y\n"
        fi
        m = m + 1
    done
    n = n + 1
done