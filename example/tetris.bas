#!/usr/bin/env bas

# Tetris for an ANSI-compatible terminal, at least 80 columns x 24 rows.
# BAS docs(3): CALL, persistent KEYMODE, zero-argument calls without ().
# Run: bas tetris.bas
# Use Q to quit so the program can restore key mode and the cursor.
# Gravity uses accumulated SLEEP time; rendering adds a little overhead.

let W = 10
let H = 20
let FRAME_US = 20000
let board = new(W * H)
let view = new(W * H)

# Four rows of four bytes per shape. O rotates in 2x2, the others in 3x3/4x4.
let shape = new(7)
shape(0) = "....XXXX........"
shape(1) = "XX..XX.........."
shape(2) = ".X..XXX........."
shape(3) = ".XX.XX.........."
shape(4) = "XX...XX........."
shape(5) = "X...XXX........."
shape(6) = "..X.XXX........."
let size = new(7)
size(0) = 4
size(1) = 2
for i = 2 to 6
    size(i) = 3
next i

let kick = new(5)
kick(0) = 0
kick(1) = -1
kick(2) = 1
kick(3) = -2
kick(4) = 2
let award = new(5)
award(1) = 100
award(2) = 300
award(3) = 500
award(4) = 800
let color = new(7)
color(0) = 36
color(1) = 33
color(2) = 35
color(3) = 32
color(4) = 31
color(5) = 34
color(6) = 37

let bag = new(7)
let bag_left = 0
print "Seed (integer; same seed gives the same pieces): "
let seed = getint
seed = ((seed % 65536) + 65536) % 65536
let piece = 0
let next_piece = 0
let rotation = 0
let px = 0
let py = 0
let over = 0
let paused = 0
let score = 0
let lines = 0
let gravity = 0
let running = 1
let dirty = 1
let escape_state = 0
let escape_age = 0

fun random_word
    seed = (25173 * seed + 13849) % 65536
    return seed
end fun

fun take_piece
    if bag_left = 0
        let i = 0
        do while i < 7
            bag(i) = i
            i = i + 1
        done
        i = 6
        do while i > 0
            let r = random_word
            let j = int(r / 256) % (i + 1)
            let t = bag(i)
            bag(i) = bag(j)
            bag(j) = t
            i = i - 1
        done
        bag_left = 7
    fi
    bag_left = bag_left - 1
    return bag(bag_left)
end fun

# Inverse rotation maps a displayed coordinate into the base shape.
fun cell(p, r, x, y)
    let n = size(p)
    if x < 0 or y < 0 or x >= n or y >= n
        return 0
    fi
    if p = 1
        r = 0
    fi
    do while r > 0
        let t = x
        x = y
        y = n - 1 - t
        r = r - 1
    done
    let pattern = shape(p)
    if pattern(y * 4 + x) = 88
        return 1
    fi
    return 0
end fun

fun fits(x, y, r)
    let b = 0
    do while b < size(piece)
        let a = 0
        do while a < size(piece)
            if cell(piece, r, a, b) != 0
                let bx = x + a
                let by = y + b
                if bx < 0 or bx >= W or by >= H
                    return 0
                fi
                if by >= 0
                    if board(by * W + bx) != 0
                        return 0
                    fi
                fi
            fi
            a = a + 1
        done
        b = b + 1
    done
    return 1
end fun

fun spawn
    piece = next_piece
    next_piece = take_piece
    rotation = 0
    px = int((W - size(piece)) / 2)
    py = 0
    # The I shape has a blank first row.
    if piece = 0
        py = -1
    fi
    gravity = 0
    if fits(px, py, rotation) = 0
        over = 1
    fi
    return 0
end fun

# Compact surviving rows downward in one pass, including adjacent full rows.
fun clear_lines
    let src = H - 1
    let dst = H - 1
    let count = 0
    do while src >= 0
        let full = 1
        let x = 0
        do while x < W
            if board(src * W + x) = 0
                full = 0
            fi
            x = x + 1
        done
        if full != 0
            count = count + 1
        else
            x = 0
            do while x < W
                board(dst * W + x) = board(src * W + x)
                x = x + 1
            done
            dst = dst - 1
        fi
        src = src - 1
    done
    do while dst >= 0
        let x = 0
        do while x < W
            board(dst * W + x) = 0
            x = x + 1
        done
        dst = dst - 1
    done
    return count
end fun

fun lock_piece
    # Check all cells before writing, so top-out cannot partially lock a piece.
    let y = 0
    do while y < size(piece)
        let x = 0
        do while x < size(piece)
            if cell(piece, rotation, x, y) != 0
                if py + y < 0
                    over = 1
                    return 0
                fi
            fi
            x = x + 1
        done
        y = y + 1
    done
    y = 0
    do while y < size(piece)
        let x = 0
        do while x < size(piece)
            if cell(piece, rotation, x, y) != 0
                board((py + y) * W + px + x) = piece + 1
            fi
            x = x + 1
        done
        y = y + 1
    done
    let n = clear_lines
    score = score + award(n) * (1 + int(lines / 10))
    lines = lines + n
    call spawn
    return 0
end fun

# Simple horizontal wall kicks; this is not the full SRS rotation system.
fun turn_piece(direction)
    let r = (rotation + direction + 4) % 4
    let i = 0
    do while i < 5
        if fits(px + kick(i), py, r) != 0
            px = px + kick(i)
            rotation = r
            return 1
        fi
        i = i + 1
    done
    return 0
end fun

fun fall_interval
    let level = int(lines / 10)
    if level >= 13
        return 100000
    fi
    return 750000 - level * 50000
end fun

fun restart
    let i = 0
    do while i < W * H
        board(i) = 0
        i = i + 1
    done
    score = 0
    lines = 0
    over = 0
    paused = 0
    bag_left = 0
    next_piece = take_piece
    call spawn
    dirty = 1
    return 0
end fun

# Start a line at an explicit screen coordinate.
# No write reaches column 80, including on the bottom row: no wrap or scroll.
fun screen_row(row)
    putchar 27
    print "[", row, ";1H"
    putchar 27
    print "[0m"
    return 0
end fun

fun erase_tail
    putchar 27
    print "[K"
    return 0
end fun

fun block(ink)
    if ink = -1
        putchar 27
        print "[90m.."
    else
        putchar 27
        print "[", color(ink - 1), "m[]"
    fi
    putchar 27
    print "[0m"
    return 0
end fun

fun sidebar(row)
    if row = 0
        print "    Score: ", score
    fi
    if row = 1
        print "    Lines: ", lines
    fi
    if row = 2
        print "    Level: ", 1 + int(lines / 10)
    fi
    if row = 4
        print "    Next:"
    fi
    if row >= 5 and row <= 8
        print "    "
        let x = 0
        do while x < 4
            if cell(next_piece, 0, x, row - 5) != 0
                call block(next_piece + 1)
            else
                print "  "
            fi
            x = x + 1
        done
    fi
    if row = 10
        print "    A/D or Left/Right: move"
    fi
    if row = 11
        print "    W / Up: rotate clockwise"
    fi
    if row = 12
        print "    X: rotate counterclockwise"
    fi
    if row = 13
        print "    S / Down: soft drop"
    fi
    if row = 14
        print "    Space: hard drop"
    fi
    if row = 15
        print "    P: pause    Q: quit"
    fi
    if row = 16
        print "    R: restart after game over"
    fi
    if row = 18
        print "    .. = landing position"
    fi
    return 0
end fun

# Overlay only the piece's small bounding box, rather than testing every cell.
fun overlay(y, ink)
    let b = 0
    do while b < size(piece)
        let a = 0
        do while a < size(piece)
            if cell(piece, rotation, a, b) != 0
                let bx = px + a
                let by = y + b
                if bx >= 0 and bx < W and by >= 0 and by < H
                    view(by * W + bx) = ink
                fi
            fi
            a = a + 1
        done
        b = b + 1
    done
    return 0
end fun

fun draw
    let i = 0
    do while i < W * H
        view(i) = board(i)
        i = i + 1
    done
    if over = 0
        let ghost = py
        do while fits(px, ghost + 1, rotation) != 0
            ghost = ghost + 1
        done
        call overlay(ghost, -1)
        call overlay(py, piece + 1)
    fi
    call screen_row(1)
    print "TETRIS / BAS"
    call erase_tail
    call screen_row(2)
    print "+--------------------+"
    call erase_tail
    let y = 0
    do while y < H
        call screen_row(y + 3)
        print "|"
        let x = 0
        do while x < W
            let ink = view(y * W + x)
            if ink = 0
                print "  "
            else
                call block(ink)
            fi
            x = x + 1
        done
        print "|"
        call sidebar(y)
        call erase_tail
        y = y + 1
    done
    call screen_row(23)
    print "+--------------------+"
    call erase_tail
    call screen_row(24)
    if over != 0
        print "GAME OVER - R: restart, Q: quit"
    else
        if paused != 0
            print "PAUSED - P: resume, Q: quit"
        else
            print "Clear lines to increase the level."
        fi
    fi
    call erase_tail
    # Flush line-buffered stdout with a newline safely at the TOP of the screen.
    # A newline at row 24 would scroll the board.
    call screen_row(1)
    print "\n"
    return 0
end fun

# Return an ordinary action byte, or 0 while consuming an escape sequence.
# Accept both CSI (ESC [) and SS3 (ESC O) arrows, including CSI parameters.
fun decode_key(ch)
    if ch = 27
        escape_state = 1
        escape_age = 0
        return 0
    fi
    if escape_state = 1
        escape_state = 0
        if ch = 91 or ch = 79
            escape_state = 2
            return 0
        fi
        # An isolated Escape must not swallow the next ordinary letter.
    else
        if escape_state = 2
            escape_age = 0
            if ch >= 64 and ch <= 126
                escape_state = 0
                if ch = 65
                    return 119
                fi
                if ch = 66
                    return 115
                fi
                if ch = 67
                    return 100
                fi
                if ch = 68
                    return 97
                fi
            else
                if ch < 32 or ch > 63
                    escape_state = 0
                fi
            fi
            return 0
        fi
    fi
    # Fold ordinary ASCII uppercase letters, after decoding arrow suffixes.
    if ch >= 65 and ch <= 90
        ch = ch + 32
    fi
    return ch
end fun

fun handle_key(ch)
    if ch = 113
        running = 0
        return 0
    fi
    if over != 0
        if ch = 114
            call restart
        fi
        return 0
    fi
    if ch = 112
        paused = 1 - paused
        gravity = 0
        dirty = 1
        return 0
    fi
    if paused != 0
        return 0
    fi
    if ch = 97
        if fits(px - 1, py, rotation) != 0
            px = px - 1
            dirty = 1
        fi
    fi
    if ch = 100
        if fits(px + 1, py, rotation) != 0
            px = px + 1
            dirty = 1
        fi
    fi
    if ch = 119
        if turn_piece(1) != 0
            dirty = 1
        fi
    fi
    if ch = 120
        if turn_piece(-1) != 0
            dirty = 1
        fi
    fi
    if ch = 115
        if fits(px, py + 1, rotation) != 0
            py = py + 1
            score = score + 1
        else
            call lock_piece
        fi
        gravity = 0
        dirty = 1
    fi
    if ch = 32
        do while fits(px, py + 1, rotation) != 0
            py = py + 1
            score = score + 2
        done
        call lock_piece
        dirty = 1
    fi
    return 0
end fun

# Enter the alternate screen and hide the cursor.
call restart
call keymode(true)
putchar 27
print "[?1049h"
putchar 27
print "[?25l"
clear
call draw
dirty = 0

do while running != 0
    # Bound input work so a key flood cannot indefinitely starve gravity.
    let count = 0
    let ch = getkey
    do while ch != 0 and running != 0 and count < 32
        let action = decode_key(ch)
        if action != 0
            call handle_key(action)
        fi
        count = count + 1
        ch = 0
        if running != 0 and count < 32
            ch = getkey
        fi
    done
    if escape_state != 0
        escape_age = escape_age + 1
        if escape_age >= 10
            escape_state = 0
            escape_age = 0
        fi
    fi
    if running != 0
        if dirty != 0
            call draw
            dirty = 0
        fi
        sleep FRAME_US
        if over = 0 and paused = 0
            gravity = gravity + FRAME_US
            if gravity >= fall_interval
                gravity = 0
                if fits(px, py + 1, rotation) != 0
                    py = py + 1
                else
                    call lock_piece
                fi
                dirty = 1
            fi
        fi
    fi
done

# Restore the original terminal before printing the final result.
call keymode(false)
putchar 27
print "[0m"
putchar 27
print "[?25h"
putchar 27
print "[?1049l"
print "Final score: ", score, "   Lines: ", lines, "\n"
