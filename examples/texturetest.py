from visual import *

# Jonathan Brandmeyer
scene.background = color.white

checkerboard = array([[0.,0.,1.,1.], 
		      [0.,0.,1.,1.],
		      [1.,1.,0.,0.],
		      [1.,1.,0.,0.]])
lum = materials.new_texture( data=checkerboard,
                         mapping="rectangular",
                         interpolate=False)
alp = materials.new_texture( data=checkerboard,
                         luminance=False,
                         mapping="rectangular",
                         interpolate=False)
balp1 = box( axis=(0,0,1), color=color.orange, material=alp)
blum1 = box( axis=(0,0,1), color=color.orange, material=lum, pos=(-2, 0))
box( pos=(0, 2), color=color.orange, opacity=0.5)
box( pos=(0,-2), color=color.orange)
blum2 = box( pos=(0, 0, -2), axis=(0,0,1), color=color.orange, material=lum)
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
lum = materials.new_texture( data=checkerboard,
                         mapping="rectangular",
                         interpolate=False)
alp = materials.new_texture( mipmap=False, data=checkerboard,
                         luminance=False,
                         mapping="rectangular",
                         interpolate=False)
balp1.material = alp
blum1.material = lum
blum2.material = lum


