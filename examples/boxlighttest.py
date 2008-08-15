from visual import *

# Mikhail Temkine, University of Toronto, April 2007

print """
Box lighting test
"""

r = 3
a1 = a2 = a3 = 0.0

arrow(pos=(0, 4, 0), axis=(0, 1, 0), color=color.red)
boxy = box(size=(3,3,3), color=(0.5, 0.5, 0.5), material=materials.rough)
b1 = sphere(radius=0.3, pos=(r, 0, 0), color=color.magenta)
b2 = sphere(radius=0.3, pos=(0, 0, r), color=color.yellow)
b3 = sphere(radius=0.3, pos=(0, 0, r), color=color.green)
l1 = light(pos=b1.pos, color=b1.color)
l2 = light(pos=b2.pos, color=b2.color)
l3 = light(pos=b3.pos, color=b3.color)

while 1:
    rate(100)
    l1.pos = b1.pos = r*vector(cos(a1), sin(a1), b1.z)
    a1 += 0.02
    l2.pos = b2.pos = (r+0.4)*vector(b2.x, sin(a2), cos(a2))
    a2 += 0.055
    l3.pos = b3.pos = (r+1)*vector(sin(a3), b3.y, cos(a3))
    a3 += 0.033
    
