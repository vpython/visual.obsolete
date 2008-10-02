from __future__ import division
from visual import *

scene.width = scene.height = 600
scene.range = 5
scene.forward = (1,-1,-1)

axis = (1,0,0)
##obj = box
##obj = cylinder; axis = (0,1,0)
obj = sphere
R = 1
L = 10

scene.visible = 0
lite = local_light( pos = (0,0,0), color = (0,1,0) )
lite.m = points( color=(0,1,0), pos=[(0,0,0)], type = "world", size = 0.05 )

spheres = []
N = int(sqrt(len(materials.materials)+1.5))
xi = -L/2 + 1.5*R
dx = (L - 3*R)/(N-1)
for i,mat in enumerate(materials.materials + [None]):
    if mat: print mat.name
    spheres.append( obj( pos = (xi + (i%N)*dx, R, (xi + (i//N)*dx)),
                         radius = R,
                         length = sqrt(2)*R,
                         height = sqrt(2)*R,
                         width = sqrt(2)*R,
                         axis = axis,
                         material = mat ) )
    
box( pos = (0,-0.5*R,0), size=(L,R,L), material = materials.wood )
scene.visible = 1

for s in spheres:
    loc = s.pos-vector(0,R,0)
    if hasattr(s.material, "name"):
        s.label = label( text = s.material.name, pos = loc )
    else:
        s.label = label( text = "Legacy", pos = loc )
    if hasattr(s.material, "color"):
        s.color = s.material.color

while 1:
    rate(100)
    for s in spheres:
        s.rotate( axis=scene.up, angle=.01 )
    lite.pos = lite.m.pos[0] = scene.mouse.project( point=scene.center, normal=scene.forward )
    if scene.mouse.clicked:
        p = scene.mouse.getclick().pick
        if p and p is not lite.m:
            scene.center = p.pos
