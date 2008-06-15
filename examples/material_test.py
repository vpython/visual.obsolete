from visual import *

axis = (1,0,0)
##obj = box
##obj = cylinder; axis = (0,1,0)
obj = sphere

spheres = []
for i,mat in enumerate(materials.materials):
    spheres.append( obj( pos = 2.2*vector( i%4-1.5, 0, i//4-1.5 ),
                         radius = 1,
                         length = sqrt(2),
                         height = sqrt(2),
                         width = sqrt(2),
                         axis = axis,
                         material = mat ) )

for s in spheres:
    s.label = label( text = s.material.name, pos = s.pos )
    if hasattr(s.material, "color"):
        s.color = s.material.color
    if hasattr(s.material, "transparent") and s.material.transparent:
        s.opacity = .999

box( pos = (0,-1.5,0), size=(10,1,10), material = materials.wood2 )

while 1:
    rate(100)
    for s in spheres:
        s.rotate( axis=scene.up, angle=.01 )
    if scene.mouse.clicked:
        p = scene.mouse.getclick().pick
        if p:
            scene.center = p.pos
