#!/usr/bin/env bas

# Tetris port for BAS
# Requires non-blocking getkey: ASCII byte when ready, 0 otherwise.
# SLEEP is in microseconds in the current BAS implementation.

let W = 10
let H = 20
let FRAME_US = 20000

# Board is flattened: board(y * W + x)
let board = new(W * H)

# Seven tetrominoes.  Each shape occupies a fixed 4x4 slot.
let shape = new(7 * 16)
let size = new(7)

size(0) = 4
size(1) = 2
size(2) = 3
size(3) = 3
size(4) = 3
size(5) = 3
size(6) = 3

# I: .... / #### / .... / ....
shape(0 * 16 + 4) = 1
shape(0 * 16 + 5) = 1
shape(0 * 16 + 6) = 1
shape(0 * 16 + 7) = 1

# O: ## / ##
shape(1 * 16 + 0) = 1
shape(1 * 16 + 1) = 1
shape(1 * 16 + 4) = 1
shape(1 * 16 + 5) = 1

# T: .#. / ### / ...
shape(2 * 16 + 1) = 1
shape(2 * 16 + 4) = 1
shape(2 * 16 + 5) = 1
shape(2 * 16 + 6) = 1

# S: .## / ##. / ...
shape(3 * 16 + 1) = 1
shape(3 * 16 + 2) = 1
shape(3 * 16 + 4) = 1
shape(3 * 16 + 5) = 1

# Z: ##. / .## / ...
shape(4 * 16 + 0) = 1
shape(4 * 16 + 1) = 1
shape(4 * 16 + 5) = 1
shape(4 * 16 + 6) = 1

# J: #.. / ### / ...
shape(5 * 16 + 0) = 1
shape(5 * 16 + 4) = 1
shape(5 * 16 + 5) = 1
shape(5 * 16 + 6) = 1

# L: ..# / ### / ...
shape(6 * 16 + 2) = 1
shape(6 * 16 + 4) = 1
shape(6 * 16 + 5) = 1
shape(6 * 16 + 6) = 1

let kick = new(5)
kick(0) = 0
kick(1) = -1
kick(2) = 1
kick(3) = -2
kick(4) = 2

let award = new(5)
award(0) = 0
award(1) = 100
award(2) = 300
award(3) = 500
award(4) = 800

let bag = new(7)
let bag_left = 0

# Deterministic LCG seed. Change this value for another piece sequence.
print "seed? "
let seed = getint

let piece = 0
let next_piece = 0
let rotation = 0
let px = 0
let py = 0
let over = 0
let paused = 0
let score = 0
let lines = 0

# Incremental ANSI arrow-key parser state.
let escape = 0

fun random_word
	seed = (25173 * seed + 13849) % 65536
	return seed
end fun

fun take_piece
	if bag_left = 0
		for i = 0 to 6
			bag(i) = i
		next i

		let i = 6
		do while i > 0
			let r = random_word
			let j = r % (i + 1)
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

# Inverse rotation: map a displayed cell back into the unrotated shape.
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

	return shape(p * 16 + y * 4 + x)
end fun

fun fits(x, y, r)
	let b = 0
	do while b < size(piece)
		let a = 0
		do while a < size(piece)
			let occupied = cell(piece, r, a, b)
			if occupied != 0
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
	py = -1

	if fits(px, py, rotation) = 0
		over = 1
	fi

	return 0
end fun

fun clear_lines
	let y = H - 1
	let count = 0

	do while y >= 0
		let full = 1
		let x = 0

		do while x < W
			if board(y * W + x) = 0
				full = 0
			fi
			x = x + 1
		done

		if full != 0
			count = count + 1

			let k = y
			do while k > 0
				let xx = 0
				do while xx < W
					board(k * W + xx) = board((k - 1) * W + xx)
					xx = xx + 1
				done
				k = k - 1
			done

			let xx = 0
			do while xx < W
				board(xx) = 0
				xx = xx + 1
			done

			# Re-check the same row after shifting.
		else
			y = y - 1
		fi
	done

	return count
end fun

fun lock_piece
	# Top-out check before modifying the board.
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
	let level = int(lines / 10)
	score = score + award(n) * (1 + level)
	lines = lines + n

	let dummy = spawn
	return 0
end fun

fun turn_piece(direction)
	let r = rotation + direction

	if r < 0
		r = r + 4
	fi
	if r >= 4
		r = r - 4
	fi

	let i = 0
	do while i < 5
		if fits(px + kick(i), py, r) != 0
			px = px + kick(i)
			rotation = r
			return 0
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
				print "[]"
			else
				print "  "
			fi
			x = x + 1
		done
	fi

	if row = 10
		print "    a/d or arrows  Move"
	fi
	if row = 11
		print "    w/up, x        Rotate CW / CCW"
	fi
	if row = 12
		print "    s/down         Soft drop"
	fi
	if row = 13
		print "    Space          Hard drop"
	fi
	if row = 14
		print "    p Pause        q Quit"
	fi
	if row = 16
		print "    Automatic gravity; faster each level."
	fi
	if row = 17
		print "    .. shows the landing position."
	fi

	if row = 19
		if over != 0
			print "    GAME OVER - press q to exit"
		else
			if paused != 0
				print "    PAUSED - press p to resume"
			fi
		fi
	fi

	return 0
end fun

fun draw
	let ghost = py

	if over = 0
		do while fits(px, ghost + 1, rotation) != 0
			ghost = ghost + 1
		done
	fi

	home
	print "TETRIS - BAS\n"
	print "+--------------------+\n"

	let y = 0
	do while y < H
		print "|"

		let x = 0
		do while x < W
			let drawn = 0

			if over = 0
				if cell(piece, rotation, x - px, y - py) != 0
					print "[]"
					drawn = 1
				fi
			fi

			if drawn = 0
				if board(y * W + x) != 0
					print "[]"
					drawn = 1
				fi
			fi

			if drawn = 0 and over = 0
				if cell(piece, rotation, x - px, y - ghost) != 0
					print ".."
					drawn = 1
				fi
			fi

			if drawn = 0
				print "  "
			fi

			x = x + 1
		done

		print "|"
		let dummy = sidebar(y)
		print "                                        \n"
		y = y + 1
	done

	print "+--------------------+\n"
	return 0
end fun

# ------------------------------------------------------------------
# Main game
# ------------------------------------------------------------------

clear

let dummy = 0
next_piece = take_piece
dummy = spawn
dummy = draw

let running = 1
let gravity = 0

# ASCII:
# a=97 d=100 w=119 x=120 s=115 p=112 q=113 space=32
# A=65  D=68  W=87  X=88  S=83  P=80  Q=81
# ESC=27, '['=91, 'O'=79, arrow A/B/C/D = 65/66/67/68

do while running != 0
	let dirty = 0
	let ch = getkey

	# Drain every key byte already waiting this frame.
	do while ch != 0
		if ch = 113 or ch = 81
			running = 0
		fi

		if running != 0
			if ch = 27
				escape = 1
			else
				if escape = 1
					if ch = 91 or ch = 79
						escape = 2
					else
						escape = 0
					fi
				else
					if escape = 2
						escape = 0
						if ch = 65
							ch = 119
						fi
						if ch = 66
							ch = 115
						fi
						if ch = 67
							ch = 100
						fi
						if ch = 68
							ch = 97
						fi
					fi

					if ch = 112 or ch = 80
						if over = 0
							if paused = 0
								paused = 1
							else
								paused = 0
							fi
							gravity = 0
							dirty = 1
						fi
					fi

					if over = 0 and paused = 0
						if ch = 97 or ch = 65
							if fits(px - 1, py, rotation) != 0
								px = px - 1
								dirty = 1
							fi
						fi

						if ch = 100 or ch = 68
							if fits(px + 1, py, rotation) != 0
								px = px + 1
								dirty = 1
							fi
						fi

						if ch = 119 or ch = 87
							dummy = turn_piece(1)
							dirty = 1
						fi

						if ch = 120 or ch = 88
							dummy = turn_piece(-1)
							dirty = 1
						fi

						if ch = 115 or ch = 83
							if fits(px, py + 1, rotation) != 0
								py = py + 1
								score = score + 1
							else
								dummy = lock_piece
								gravity = 0
							fi
							dirty = 1
						fi

						if ch = 32
							do while fits(px, py + 1, rotation) != 0
								py = py + 1
								score = score + 2
							done
							dummy = lock_piece
							gravity = 0
							dirty = 1
						fi
					fi
				fi
			fi
		fi

		ch = getkey
	done

	if running != 0
		if over = 0 and paused = 0
			gravity = gravity + FRAME_US

			if gravity >= fall_interval
				if fits(px, py + 1, rotation) != 0
					py = py + 1
				else
					dummy = lock_piece
				fi
				gravity = 0
				dirty = 1
			fi
		fi

		if dirty != 0
			dummy = draw
		fi

		sleep FRAME_US
	fi
done

clear
print "Final score: ", score, "   Lines: ", lines, "\n"
