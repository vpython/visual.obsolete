from visual import *

scene.show_rendertime = 1
scene.foreground = (1,1,1)

axis = (1,0,0)
##obj = box
##obj = cylinder; axis = (0,1,0)
obj = sphere

##scene.lights = []
lite = light( pos = (0,0,0), color = (0,1,0),
              specular_color = (0,1,0), local = True )
lite.m = points( color=(0,1,0), pos=[(0,0,0)], type = "world", size = 0.05 )

spheres = []
for i,mat in enumerate(materials.materials + [None]):
    if mat: print mat.name
    spheres.append( obj( pos = 2.2*vector( i%4-1.5, 0, i//4-1.5 ),
                         radius = 1,
                         length = sqrt(2),
                         height = sqrt(2),
                         width = sqrt(2),
                         axis = axis,
                         material = mat ) )
    scene.visible = 1
    
box( pos = (0,-1.5,0), size=(10,1,10), material = materials.wood )

for s in spheres:
    if hasattr(s.material, "name"):
        s.label = label( text = s.material.name, pos = s.pos )
    else:
        s.label = label( text = "Legacy", pos = s.pos )
    if hasattr(s.material, "color"):
        s.color = s.material.color
    if hasattr(s.material, "transparent") and s.material.transparent:
        s.opacity = .999


while 1:
    rate(100)
    for s in spheres:
        s.rotate( axis=scene.up, angle=.01 )
    lite.pos = lite.m.pos[0] = scene.mouse.project( point=scene.center, normal=scene.forward )
    if scene.mouse.clicked:
        p = scene.mouse.getclick().pick
        if p and p is not lite.m:
            scene.center = p.pos
