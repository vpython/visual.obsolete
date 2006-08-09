from visual import *

# The type of the texture is inferred from its number of dimentions.  1 is 
# either a luminance or alpha map (default:luminance), 2 is luminance-alpha,
# 3 is RGB, 4 is RGBA
# The primitive data type can be a char, int, long, short, float, or double.
# however, int types are converted from 0-255 -> 0.->1.

# Jonathan Brandmeyer

checkerboard = array([[0.,0.,1.,1.], 
					  [0.,0.,1.,1.],
					  [1.,1.,0.,0.],
					  [1.,1.,0.,0.]])
lum = texture( data=checkerboard, type = "luminance")
checkerboard = lum.data # in case a copy was made
# By default, textures are mipmapped automatically.  The user should set it to
# false if the texture will be changed frequently.
alp = texture( mipmap = False, data=checkerboard, type = "opacity")
box( color=color.orange, texture=alp)
box( color=color.orange, texture=lum, pos=(-2, 0))
box( pos=(0, 2), color=color.orange, opacity=0.5)
box( pos=(0,-2), color=color.orange)
box( pos=(0, 0, -2), color=color.orange, lit=False, texture=lum)
z = arrow()
z.axis *= 2

# Changes in the display are a consequence of changing the underlying data.
scene.mouse.getclick()
print "click!"
save = array(checkerboard[:,1])
checkerboard[:,1] = checkerboard[:,2]
checkerboard[:,2] = save
save = array(checkerboard[1,:])
checkerboard[1,:] = checkerboard[2,:]
checkerboard[2,:] = save

