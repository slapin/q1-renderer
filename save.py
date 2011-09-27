fd = open("viewbuf")
fd2 = open("viewbuf.txt", "w")
w = 1280
h = 1024

for k in range(0, 1024):
	l = fd.read(w)
	for c in l:
		h = ord(c)
		if h > 0 and h <= 0x20:
			h = "."
		elif h > 0x20 and  h < 0x7f:
			h = chr(h)
		elif h > 0x7f:
			h = "X"
		else:
			h = " "
		fd2.write(h)
	fd2.write("\n")

fd2.close()

