from visual import *

# Jonathan Brandmeyer

checkerboard = array([[0.,0.,1.,1.], 
					  [0.,0.,1.,1.],
					  [1.,1.,0.,0.],
					  [1.,1.,0.,0.]])
lum = materials.texture( data=checkerboard, channels=["luminance"], mapping="rectangular")
alp = materials.texture( data=checkerboard, channels=["opacity"], mapping="rectangular")
balp1 = box( color=color.orange, material=alp)
blum1 = box( color=color.orange, material=lum, pos=(-2, 0))
box( pos=(0, 2), color=color.orange, opacity=0.5)
box( pos=(0,-2), color=color.orange)
blum2 = box( pos=(0, 0, -2), color=color.orange, lit=False, material=lum)
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
lum = materials.texture( data=checkerboard,
                         channels=["luminance"], mapping="rectangular")
alp = materials.texture( mipmap = False, data=checkerboard,
                         channels=["opacity"], mapping="rectangular")
balp1.material = alp
blum1.material = lum
blum2.material = lum


