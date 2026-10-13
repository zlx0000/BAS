fun gcd(a, b)
 if b == 0
  return a
 fi
 let r = a % b
 return gcd(b, r)
end fun

print gcd(1071, 462), "\n"