from visual import *
checkerboard = array([[[0.],[0.],[1.],[1.]], 
					  [[0.],[0.],[1.],[1.]],
					  [[1.],[1.],[0.],[0.]],
					  [[1.],[1.],[0.],[0.]]])
lum = texture( mipmapped = False, data=checkerboard, type = "luminance")
alp = texture( mipmapped = False, data=checkerboard, type = "alpha")
box( color=color.orange, texture=alp)
box( color=color.orange, texture=lum, pos=(-2, 0))
box( pos=(0, 2), color=color.orange, alpha=0.5)
box( pos=(0,-2), color=color.orange)
box( pos=(0, 0, -2), color=color.orange, lit=False, texture=lum)
z = arrow()
z.axis *= 2
