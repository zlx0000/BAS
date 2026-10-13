#!/usr/bin/env bas

# Recursive descent calculator with a getchar REPL.
# Like getint, getchar is called as a bare identifier.
# parse accepts a string or a character-code array, plus its length.
# Grammar:
# expression = term { ("+" | "-") term }
# term       = unary { ("*" | "/") unary }
# unary      = ("+" | "-") unary | primary
# primary    = number | "(" expression ")"
# number     = digits ["." digits] | "." digits

let source = ""
let limit = 0
let pos = 0
let error = ""
let error_pos = 0
let failed = true

fun peek
    if pos >= limit
        return 0
    fi
    return source(pos)
end fun

fun fail(message)
    if not failed
        failed = true
        error = message
        error_pos = pos
    fi
    return 0
end fun

fun skip_space
    do while peek = 32 or peek = 9 or peek = 10 or peek = 13
        pos = pos + 1
    done
    return 0
end fun

fun digit(c)
    return c >= 48 and c <= 57
end fun

# Node layout: [kind, left/value, right, source_position].
# kind 0 = number, 43/45/42/47 = binary + - * /,
# kind -43/-45 = unary +/-. Missing children are 0.
fun node(kind, left, right, at)
    let tree = new(4)
    tree(0) = kind
    tree(1) = left
    tree(2) = right
    tree(3) = at
    return tree
end fun

fun number
    let at = pos
    let value = 0.0
    let count = 0
    do while digit(peek)
        value = value * 10 + peek - 48
        pos = pos + 1
        count = count + 1
    done
    if peek = 46
        pos = pos + 1
        let place = 0.1
        do while digit(peek)
            value = value + (peek - 48) * place
            place = place / 10
            pos = pos + 1
            count = count + 1
        done
    fi
    if count = 0
        return fail("Expected a number or '('")
    fi
    return node(0, value, 0, at)
end fun

fun primary
    let ignored = skip_space
    if peek = 40
        pos = pos + 1
        let value = expression
        if failed
            return 0
        fi
        ignored = skip_space
        if peek != 41
            return fail("Expected ')'")
        fi
        pos = pos + 1
        return value
    fi
    return number
end fun

fun unary
    let ignored = skip_space
    if peek = 43 or peek = 45
        let op = peek
        let at = pos
        pos = pos + 1
        let child = unary
        if failed
            return 0
        fi
        return node(-op, child, 0, at)
    fi
    return primary
end fun

fun term
    let value = unary
    let ignored = skip_space
    do while (not failed) and (peek = 42 or peek = 47)
        let op = peek
        let at = pos
        pos = pos + 1
        let rhs = unary
        if failed
            return 0
        fi
        value = node(op, value, rhs, at)
        ignored = skip_space
    done
    return value
end fun

fun expression
    let value = term
    let ignored = skip_space
    do while (not failed) and (peek = 43 or peek = 45)
        let op = peek
        let at = pos
        pos = pos + 1
        let rhs = term
        if failed
            return 0
        fi
        value = node(op, value, rhs, at)
        ignored = skip_space
    done
    return value
end fun

# Pass the exact number of characters in text.
# On failure returns 0 and sets failed, error, and error_pos.
fun parse(text, length)
    source = text
    limit = length
    pos = 0
    failed = false
    error = ""
    error_pos = 0
    let value = expression
    let ignored = skip_space
    if (not failed) and pos != limit
        ignored = fail("Unexpected character")
    fi
    if failed
        return 0
    fi
    return value
end fun

# Track which ancestors still have a following child.
let d_arr = new(21)

fun print_tree(t, d)
    if d > 20
        print "... (depth limit)\n"
        t = 0
        return -1
    fi

    let kind = t(0)
    if kind = 0
        print t(1), "\n"
        t = 0
        return 0
    fi
    if kind = -43
        print "pos\n"
    else
        if kind = -45
            print "neg\n"
        else
            putchar kind
            print "\n"
        fi
    fi

    # A unary operator has only its left child.
    let last = 2
    if kind < 0
        last = 1
    fi
    for i = 1 to last
        if t(i) != 0
            let j = 1
            do while j < d
                if d_arr(j) = 1
                    print "│ "
                else
                    print "  "
                fi
                j = j + 1
            done
            if i != last
                print "├─"
                d_arr(d) = 1
            else
                print "└─"
                d_arr(d) = 0
            fi
            let ignored = print_tree(t(i), d + 1)
        fi
    next i
    t = 0
    return 0
end fun

# Evaluation is separate from parsing.
fun evaluate(tree)
    let kind = tree(0)
    if kind = 0
        return tree(1)
    fi
    let left = evaluate(tree(1))
    if failed
        return 0
    fi
    if kind = -43
        return left
    fi
    if kind = -45
        return -left
    fi
    let right = evaluate(tree(2))
    if failed
        return 0
    fi
    if kind = 43
        return left + right
    fi
    if kind = 45
        return left - right
    fi
    if kind = 42
        return left * right
    fi
    if right = 0
        pos = tree(3)
        return fail("Division by zero")
    fi
    return left / right
end fun

fun calc(text, length)
    let tree = parse(text, length)
    if failed
        return 0
    fi
    return evaluate(tree)
end fun

fun show(text, length)
    let tree = parse(text, length)
    if not failed
        print "AST:\n"
        let ignored = print_tree(tree, 1)
        let result = evaluate(tree)
        if not failed
            print "Result: ", result, "\n"
        fi
    fi
    if failed
        print "Error at column ", error_pos + 1, ": ", error, "\n"
    fi
    return 0
end fun

# Read one expression per line. An empty line or EOF exits.
# Lines longer than 256 characters are discarded.
let capacity = 256
let buffer = new(capacity)
let running = 1
print "Calculator: + - * / and parentheses. Blank line exits.\n"
do while running
    print "> "
    let used = 0
    let overflow = false
    let ch = getchar
    do while ch != 10 and ch != -1
        if used < capacity
            buffer(used) = ch
            used = used + 1
        else
            overflow = true
        fi
        ch = getchar
    done
    if overflow
        print "Error: line exceeds 256 characters\n"
    else
        if used = 0
            running = false
        else
            let ignored = show(buffer, used)
        fi
    fi
    if ch = -1
        running = false
    fi
    # Collect completed and partially parsed trees after each line.
    gc
done
free buffer
