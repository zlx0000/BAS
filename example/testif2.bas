let f=1
goto 10
1 let a=3
f=0

if a<=1
	print 1
else
	if a<=2
		print 2
	else
		if a<=3
			print 3
		else
			if a<=4
				print 4
			else
				if a<=5
					print 5
				fi
			fi
		fi
	fi
fi
10 if f then 1
