#!/usr/bin/env bas

# Interactive Mandelbrot viewer for BAS.
# Requires an ANSI terminal with 256 colours and UTF-8, at least 80x24.
# Run: bas mandelbrot.bas
# WASD/arrows move; +/- or Z/X zoom; ]/[ change iterations; R resets; Q quits.
# Zoom is centred on the middle of the picture.
# Uses 78 columns x 20 rows, with two vertical samples per terminal cell.
# Panning reuses overlapping samples and calculates only newly exposed strips.
# Rendering resumes from cached samples after input; unchanged cells are not redrawn.
# Zoom changes the sample grid, so it requires fresh calculations.
# Quit with Q so key mode, colours, cursor and alternate screen are restored.

let W = 78
let H = 20
let cx = -0.5
let cy = 0.0
let span = 3.5
let limit = 192
let running = 1
let dirty = 1
let key = 0
let escape_state = 0

# One iteration count and its calculation limit per vertical sample.
# A zero limit marks a sample that has not been calculated yet.
let counts = new(W * H * 2)
let caps = new(W * H * 2)
let spare_counts = new(W * H * 2)
let spare_caps = new(W * H * 2)
let screen_upper = new(W * H)
let screen_lower = new(W * H)

# Whole sample steps preserve the grid when panning (about 15% of the view).
let step_x = int(W * 0.15 + 0.5)
let step_y = int(2 * H * 0.15 + 0.5)

# 256-colour palette: deep blue through cyan, yellow and orange.
let palette = new(16)
palette(0) = 17
palette(1) = 18
palette(2) = 19
palette(3) = 20
palette(4) = 21
palette(5) = 27
palette(6) = 33
palette(7) = 39
palette(8) = 45
palette(9) = 51
palette(10) = 87
palette(11) = 123
palette(12) = 229
palette(13) = 221
palette(14) = 214
palette(15) = 208

fun cursor(row, col)
    print "\27[", row, ";", col, "H"
    return 0
end fun

# Return the number of iterations before escape, capped at limit.
# No interior shortcuts: every sample exercises the iteration loop.
fun escape_count(cr, ci)
    let zr = 0.0
    let zi = 0.0
    let rr = 0.0
    let ii = 0.0
    let n = 0
    do while n < limit and rr + ii <= 4.0
        zi = 2.0 * zr * zi + ci
        zr = rr - ii + cr
        rr = zr * zr
        ii = zi * zi
        n = n + 1
    done
    return n
end fun

fun colour(n)
    if n >= limit
        return 16
    fi
    return palette(int(n / 3) % 16)
end fun

# Keep escaped samples across iteration changes. Interior samples need another
# calculation only when the requested limit exceeds their previous limit.
fun sample(index, cr, ci)
    if caps(index) > 0
        if counts(index) < caps(index) or caps(index) >= limit
            return colour(counts(index))
        fi
    fi
    counts(index) = escape_count(cr, ci)
    caps(index) = limit
    return colour(counts(index))
end fun

fun invalidate
    let i = 0
    do while i < W * H * 2
        caps(i) = 0
        i = i + 1
    done
    return 0
end fun

# dx points right, dy points down in the sample grid. Use separate buffers so
# copying never overwrites a source sample, including after an interrupted render.
fun pan(dx, dy)
    let y = 0
    do while y < 2 * H
        let x = 0
        do while x < W
            let target = y * W + x
            let sx = x + dx
            let sy = y + dy
            spare_caps(target) = 0
            if sx >= 0 and sx < W and sy >= 0 and sy < 2 * H
                let source = sy * W + sx
                spare_counts(target) = counts(source)
                spare_caps(target) = caps(source)
            fi
            x = x + 1
        done
        y = y + 1
    done

    let old_counts = counts
    let old_caps = caps
    counts = spare_counts
    caps = spare_caps
    spare_counts = old_counts
    spare_caps = old_caps

    cx = cx + dx * (span / W)
    cy = cy - dy * (span / W)
    dirty = 1
    return 0
end fun

fun status(message)
    call cursor(21, 1)
    print "\27[0m\27[2K"
    print "Centre ", cx, ", ", cy

    call cursor(22, 1)
    print "\27[2K"
    print "Zoom ", 3.5 / span, "x  Iter ", limit

    call cursor(23, 1)
    print "\27[2K"
    print "WASD/arrows move  +/- zoom  [/] iterations  R reset  Q quit"

    call cursor(24, 1)
    print "\27[2K", message

    # Flush line-buffered terminal output without scrolling the bottom row.
    call cursor(23, 79)
    print "\13\n"
    return 0
end fun

fun render
    call status("Rendering...  Zoom targets the centre of the picture.")

    let scale = span / W
    let left = cx - span / 2.0
    let top = cy + scale * H

    let y = 0
    do while y < H
        let drawing = 0
        let ci = top - (2 * y + 0.5) * scale
        let x = 0

        do while x < W
            let cr = left + (x + 0.5) * scale
            let index = 2 * y * W + x
            let upper = sample(index, cr, ci)
            let lower = sample(index + W, cr, ci - scale)
            let cell = y * W + x

            if upper != screen_upper(cell) or lower != screen_lower(cell)
                if drawing = 0
                    call cursor(y + 1, x + 1)
                fi

                print "\27[38;5;", upper, "m"
                print "\27[48;5;", lower, "m▀"

                screen_upper(cell) = upper
                screen_lower(cell) = lower
                drawing = 1
            else
                drawing = 0
            fi

            x = x + 1
        done

        print "\27[0m"
        call cursor(y + 1, W + 1)
        print "\13\n"

        let pressed = getkey
        if pressed != 0
            dirty = 1
            return pressed
        fi

        y = y + 1
    done

    call status("Ready.  Move towards an edge of the set, then zoom in.")
    return 0
end fun

call keymode(true)

# Alternate screen, hide cursor and clear once.
print "\27[?1049h\27[?25l\27[2J"

do while running
    if dirty = 1 and escape_state = 0
        dirty = 0
        key = render
    else
        key = getkey
    fi

    # Decode both CSI and SS3 arrow sequences without blocking.
    if key != 0
        if escape_state = 0
            if key = 27
                escape_state = 1
                key = 0
            fi
        else
            if escape_state = 1
                escape_state = 0
                if key = 91 or key = 79
                    escape_state = 2
                    key = 0
                fi
            else
                escape_state = 0
                if key = 65
                    key = 119
                else
                    if key = 66
                        key = 115
                    else
                        if key = 67
                            key = 100
                        else
                            if key = 68
                                key = 97
                            else
                                key = 0
                            fi
                        fi
                    fi
                fi
            fi
        fi

        # Treat upper-case letters like their lower-case counterparts.
        if key >= 65 and key <= 90
            key = key + 32
        fi

        if key = 113
            running = 0
        fi

        if key = 119
            call pan(0, 0 - step_y)
        fi

        if key = 115
            call pan(0, step_y)
        fi

        if key = 97
            call pan(0 - step_x, 0)
        fi

        if key = 100
            call pan(step_x, 0)
        fi

        if key = 43 or key = 61 or key = 122
            span = span / 1.5
            call invalidate
            dirty = 1
        fi

        if key = 45 or key = 95 or key = 120
            let previous_span = span
            span = span * 1.5

            if span > 8.0
                span = 8.0
            fi

            if span != previous_span
                call invalidate
                dirty = 1
            fi
        fi

        if key = 93
            limit = limit * 2
            if limit > 3072
                limit = 3072
            fi
            dirty = 1
        fi

        if key = 91
            limit = int(limit / 2)
            if limit < 24
                limit = 24
            fi
            dirty = 1
        fi

        if key = 114
            cx = -0.5
            cy = 0.0
            span = 3.5
            limit = 192
            call invalidate
            dirty = 1
        fi
    fi

    if key = 0
        sleep 10000
    fi
done

call keymode(false)

# Reset colours, restore cursor and return to the normal screen.
print "\27[0m\27[?25h\27[?1049l\n"

end