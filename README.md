# BAS — BAS Array Script

**BAS** is a small interpreted scripting language written in C.

The name is a recursive acronym:

> **BAS = BAS Array Script**

BAS began as a BASIC-like interpreter and evolved into a small general-purpose language centered around simple syntax and nested arrays.

## Features

- Functions and recursion
- `if / else / fi`
- `for / next` and `do while / done`
- Nested arrays with `new(n)`
- Arrays usable as trees, lists and graphs
- Automatic tracing garbage collection
- Interactive REPL
- Script execution with `#!/usr/bin/env bas`

## Example

Nested arrays can be used to build recursive data structures such as binary trees:

```bas
#!/usr/bin/env bas

let v = 0
let l = 1
let r = 2

fun build(x)
	let n = new(3)
	n(v) = x
	n(l) = 0
	n(r) = 0

	if x > 1
		n(l) = build(x - 1)
		n(r) = build(x - 1)
	fi

	return n
end fun

fun print_tree(t)
	print t(v), "\n"

	for i = 1 to 2
		if t(i) != 0
			let _ = print_tree(t(i))
		fi
	next i

	return 0
end fun

let tree = build(4)
let _ = print_tree(tree)

free tree
```

BAS is an experimental language for exploring interpreters, parsing, runtime design, and memory management.