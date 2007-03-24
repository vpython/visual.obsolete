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
a3 = 0.0

arrow(pos=(0, 4, 0), axis=(0, 1, 0), color=color.red)
wood = texture(data=array(pickle.load(open('wood256lum.vpt', 'rU'))))
boxy = box(size=(3,3,3), color=(0.5, 0.5, 0.5), texture=wood)
b1 = sphere(radius=0.3, pos=(r, 0, 0), color=color.magenta)
b2 = sphere(radius=0.3, pos=(0, 0, r), color=color.yellow)
b3 = sphere(radius=0.3, pos=(0, 0, r), color=color.green)
l1 = light(pos=(r, 0, 0), color=color.magenta)#, spot_direction=(-1,0,0), spot_cutoff=20)
l2 = light(pos=(0, 0, r), color=color.yellow)#, spot_direction=(0,0,-1), spot_cutoff=10)
l3 = light(pos=(0, 0, r), color=color.green)#, spot_direction=(0,0,-1), spot_cutoff=10)


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
    a1 += 0.02
    b2.pos.z = (r+0.4)*cos(a2)
    b2.pos.y = (r+0.4)*sin(a2)
    l2.pos.z = (r+0.4)*cos(a2)
    l2.pos.y = (r+0.4)*sin(a2)
    a2 += 0.055
    b3.pos.z = (r+1)*cos(a3)
    b3.pos.x = (r+1)*sin(a3)
    l3.pos.z = (r+1)*cos(a3)
    l3.pos.x = (r+1)*sin(a3)
    a3 += 0.033
    