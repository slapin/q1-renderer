import os
def drange(start, stop, step):
    r = start
    while r < stop:
            yield r
            r += step
            


def runpoly(*args):
	r = os.system("./poly " + " ".join(args[1:]))
	if r != 0:
		fd = open("fault.log", "a")
		fd.write("./poly " + " ".join(args[1:]))
		fd.close()
		os.system("gdb --args ./poly " + " ".join(args[1:]))
		return
	if os.path.exists("viewbuf"):
		fd = open("viewbuf")
		l = fd.read()
		nt = 0
		for c in l:
			if ord(c) != 0:
				nt = nt + 1
		fd.close()
		if nt > 0x800:
			os.system("convert -size 1280x1024 -depth 8 gray:viewbuf -depth 24 -size 320x240 " + args[0] + ".png")

for i in drange(-2.5, 2.5, 0.1):
	for j in drange(-2.5, 2.5, 0.1):
		for k in drange(-3500.0, 0, 0.4):
			i = str(i)
			j = str(j)
			k = str(k)
			runpoly("test_" + i + "+" + j + "+" + k, i, j, k)

