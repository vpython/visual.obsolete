from visual import *

# The type of the texture is inferred from its number of dimensions.  1 is 
# either a luminance or opacity map (default:luminance), 2 is luminance-opacity,
# 3 is RGB, 4 is RGB-opacity.
# The primitive data type can be a char, int, long, short, float, or double.
# however, int types are converted from 0-255 -> 0.->1.

# Jonathan Brandmeyer

checkerboard = array([[0.,0.,1.,1.], 
					  [0.,0.,1.,1.],
					  [1.,1.,0.,0.],
					  [1.,1.,0.,0.]])
lum = texture( data=checkerboard, type = "luminance")
checkerboard = lum.data # in case a copy was made
# By default, textures are mipmapped automatically. The programmer should set
# it to false if the texture will be changed frequently.
alp = texture( mipmap = False, data=checkerboard, type = "opacity")
balp1 = box( color=color.orange, texture=alp)
blum1 = box( color=color.orange, texture=lum, pos=(-2, 0))
box( pos=(0, 2), color=color.orange, opacity=0.5)
box( pos=(0,-2), color=color.orange)
blum2 = box( pos=(0, 0, -2), color=color.orange, lit=False, texture=lum)
z = arrow()
z.axis *= 2

scene.mouse.getclick()
# Change the texture
save = array(checkerboard[:,1])
checkerboard[:,1] = checkerboard[:,2]
checkerboard[:,2] = save
save = array(checkerboard[1,:])
checkerboard[1,:] = checkerboard[2,:]
checkerboard[2,:] = save
# Recreate and reassign the textures; Visual doesn't check for texture changes
lum = texture( data=checkerboard, type = "luminance")
alp = texture( mipmap = False, data=checkerboard, type = "opacity")
balp1.texture = alp
blum1.texture = lum
blum2.texture = lum


