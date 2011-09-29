import os
def drange(start, stop, step):
    r = start
    while r < stop:
            yield r
            r += step
            


def runpoly(*args):
	args = map(str, args)
	cmd = "./poly -f " + args[0] + " -- " + " ".join(args[1:])
	if os.path.exists("fault.log"):
		os.unlink("fault.log")
	r = os.system(cmd)
	if r != 0:
		fd = open("fault.log", "a")
		fd.write(cmd)
		fd.close()
		os.system("gdb --args " + cmd)
		return

#for i in drange(20.0, 30.0, 0.5):
#	for j in drange(-125.0, -120.0, 0.5):
#		for k in drange(13.0, 18.0, 0.5):
#			runpoly("test_" + str(i) + "+" + str(j) + "+" + str(k), 3.0, 3.0, i, j, k)

#runpoly("view", 3.0, 3.0, 25.5, -123.5, 15.0)

defx = 25.5
defy = -123.5
defz = 15.0
vrightx = 10.0
vrighty = 0.0
vrightz = 0.0
vupx = 0.0
vupy = 10.0
vupz = 0.0
def xorigintest():
	for x in drange(20.0, 30.0, 1.5):
		runpoly("xorigintest_" + str(x), 3.0, 3.0, x, defy, defz, vrightx, vrighty, vrightz, vupx, vupy, vupz)
def xvrighttest():
	for x in drange(0.1, 400.0, 5.0):
		runpoly("xvrighttest_" + str(x), 3.0, 3.0, defx, defy, defz, x, vrighty, vrightz, vupx, vupy, vupz)
#def yvrighttest():
#	for y in drange(0.1, 400.0, 5.0):
#		runpoly("yvrighttest_" + str(y), 3.0, 3.0, defx, defy, defz, vrightx, y, vrightz, vupx, vupy, vupz)
def zvrighttest():
	for z in drange(-400, 400.0, 5.0):
		runpoly("zvrighttest_" + str(z), 3.0, 3.0, defx, defy, defz, vrightx, vrighty, z, vupx, vupy, vupz)
def xvuptest():
	for x in drange(0.1, 44.0, 5.0):
		runpoly("xvuptest_" + str(x), 3.0, 3.0, defx, defy, defz, vrightx, vrighty, vrightz, x, vupy, vupz)
def yvuptest():
	for y in drange(0.1, 83.0, 5.0):
		runpoly("yvuptest_" + str(y), 3.0, 3.0, defx, defy, defz, vrightx, vrighty, vrightz, vupx, y, vupz)
def zvuptest():
	for z in drange(0.1, 69.0, 5.0):
		runpoly("zvuptest_" + str(z), 3.0, 3.0, defx, defy, defz, vrightx, vrighty, vrightz, vupx, vupy, z)
	
xorigintest()
xvrighttest()
#yvrighttest()
zvrighttest()
xvuptest()
yvuptest()
zvuptest()

