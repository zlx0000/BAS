for _ = 1 to 2
print _
let n = 50
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
11 stack(top)=cur
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
print d(nxt)
free d
next _