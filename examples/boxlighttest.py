from visual import *
import pickle

print """
Box lighting test
Correct surfaces should appear shiny and polished; incorrect are dull ones
Click to switch between a textured and non-textured box
"""

r = 3
a1 = 0.0
a2 = 0.0

wood = texture(data=array(pickle.load(open('wood256lum.vpt', 'rU'))))
boxy = box(size=(3,3,3), color=(0.5, 0.5, 0.5), texture=wood)
b1 = sphere(radius=0.3, pos=(r, 0, 0), color=color.magenta)
b2 = sphere(radius=0.3, pos=(0, 0, r), color=color.yellow)
l1 = light(pos=(r, 0, 0), color=color.magenta)#, spot_direction=(-1,0,0), spot_cutoff=20)
l2 = light(pos=(0, 0, r), color=color.yellow)#, spot_direction=(0,0,-1), spot_cutoff=10)

while 1:
    if scene.mouse.clicked:
        scene.mouse.getclick()
        if boxy.texture == None:
            boxy.texture=wood
        else:
            boxy.texture=None
    rate(100)
    b1.pos.x = r*cos(a1)
    b1.pos.y = r*sin(a1)
    l1.pos.x = r*cos(a1)
    l1.pos.y = r*sin(a1)
    #l1.spot_direction.x = -cos(a1)
    #l1.spot_direction.y = -sin(a1)
    a1 += 0.02
    b2.pos.z = (r+0.4)*cos(a2)
    b2.pos.y = (r+0.4)*sin(a2)
    l2.pos.z = (r+0.4)*cos(a2)
    l2.pos.y = (r+0.4)*sin(a2)
    #l2.spot_direction.z = -cos(a1)
    #l2.spot_direction.y = -sin(a1)
    a2 += 0.055
    