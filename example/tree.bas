#!/usr/bin/env bas

let v = 0
let l = 1
let r = 2

fun build(x)
	if x = 1
		let n = new(3)
		n(l)=0
		n(r)=0
		n(v)=x
	else
		let n = new(3)
		n(l)=build(x-1)
		n(r)=build(x-1)
		n(v)=x
	fi
	return n
end fun

let d_arr = new(20)

fun print_tree(t, d)
	if d > 20
		t = 0
		return -1
	fi

	print t(v), "\n"

	for i = 1 to 2
		if t(i) != 0
			let j = 1

			do while j < d
				if d_arr(j) == 1
					print "│ "
				else
					print "  "
				fi
				j = j + 1
			done

			if i != 2
				print "├─"
				d_arr(d) = 1
			else
				print "└─"
				d_arr(d) = 0
			fi

			let _ = print_tree(t(i), d+1)
		fi
	next i
	t = 0
	return 0
end fun

1 print "depth? "
let d = getint
if d > 20 or d < 1
	free d_arr
	end
fi
let tree = build(d)
let _ = print_tree(tree, 1)
free tree
goto 1

