from visual import *
# Kadir Haldenbilen, February 2011

scene.width = scene.height = 800
scene.forward = (0.2,-0.6,-0.8)
scene.title = "Differential Gear"

print ("Click to pause or restart.")

def scaler(start=(1.,1.), end=(1.,1.), np=2):
    sl = []
    for i in range(np):
        sl.append((start[0]+(end[0]-start[0])/(np-1)*i,
                   start[1]+(end[1]-start[1])/(np-1)*i))
    return sl
 
def bevelGears(R1=5.0, n1=15, t1=2.0, GR=2.0, hole1=False, hole2=False, twist=0.0):
    # Gear 2 radius and teeth numbers
    R2 = GR*R1
    n2 = int(GR*n1)

    # Calculate the thickness of gear 2, and the scaling factor
    r2 = R2-t1      # final radius of gear 2
    r1 = (R1/R2)*r2 # final radius of gear 1
    t2 = R1-r1      # thickness of gear 2
    scaling = r1/R1 # both extrusions are scaled by this factor

    g1 = shapes.gear(n=n1, radius=R1)
    if hole1: g1 -= shapes.circle(radius=R1/2.)
    g2 = shapes.gear(n=n2, radius=R2)
    if hole2: g2 -= shapes.circle(radius=R2/2.)

    lnp = 2
    if twist: lnp = 8
        
    cfrm = frame()
    frm1 = frame(frame=cfrm)
    eg1 = extrusion(shape=g1, pos=paths.line(start=(0,0,0), end=(0,0,t1), np=lnp),
                   scale=scaler(start=(1,1),end=(scaling,scaling), np=lnp),
                    twist=-twist, frame=frm1)

    frm2 = frame(frame=cfrm)
    eg2 = extrusion(shape=g2, pos=paths.line(start=(R1,0,R2), end=(R1-t2,0,R2), np=lnp),
                   scale=scaler(start=(1,1),end=(scaling,scaling), np=lnp),
                    twist=twist/GR, frame=frm2, color=color.red)
    if n2%n1: eg2.frame.rotate(axis=(1,0,0), angle=pi/n2, origin=(0,0,R2))

    return R1, n1, R2, n2, eg1, eg2

R1 = 3.
n1 = 8
t1 = 3.
GR1 = 1.5

R5 = 5.
n5 = 15
t5 = 4.
GR3 = 3.

R1, n1, R2, n2, eg1, eg2  = bevelGears(R1=R1, n1=n1, t1=t1, GR=GR1, twist=0.0)
R3, n1, R4, n2, eg3, eg4  = bevelGears(R1=R1, n1=n1, t1=t1, GR=GR1, twist=0.0)

f1 = eg1.frame.frame
f3 = eg3.frame.frame
f3.rotate(axis=(0,1,0), angle=pi)

R5, n5, R6, n6, eg5, eg6  = bevelGears(R1=R5, n1=n5, t1=t5, GR=GR3, hole2=True, twist=0.0)
eg5.color = (1,1,0)
eg6.color = (0,1,1)

f6 = eg6.frame.frame
p1 = box(frame=eg6.frame, pos=(0,0,R6/2), size=(R5*2*0.7,R5,R5/4),
         color=(0,0,1), opacity=0.5)
p2 = box(frame=eg6.frame, pos=(0,0,R6*3./2.), size=(R5*2*0.7,R5,R5/4),
         color=(0,0,1), opacity=0.5)
shft = cylinder(frame=eg6.frame, pos=p1.pos, axis=p2.pos-p1.pos, radius=0.5,
                color=(0,1,0))
dsk = extrusion(frame=eg6.frame, pos=[(5,0,R6),(5.5,0,R6)],
               color=eg6.color[0], shape=(shapes.circle(radius=R6+0.4) -
                shapes.circle(radius=R6/2.)))
f6.pos = (0,0,-(R6-R2))

f1.frame = eg6.frame
f3.frame = eg6.frame
f3.pos = (0,0, R6+t1*1.5)
f1.pos = (0,0, R6-t1*1.5)


mshaft = extrusion(shape=shapes.ngon(np=8, radius=R5/2.), color=eg5.color[0]*0.5,
                   pos=[-eg5.pos[-1]*5, eg5.pos[0]], frame=eg5.frame)
raxis = extrusion(shape=shapes.ngon(np=8, radius=R2/4.), color=eg2.color[0]*0.5,
                   pos=[eg2.pos[0], eg2.pos[0]+eg2.frame.axis*15], frame=eg2.frame)
laxis = extrusion(shape=shapes.ngon(np=8, radius=R4/4.), color=eg4.color[0]*0.5,
                   pos=[eg4.pos[0], eg4.pos[0]+eg4.frame.axis*15], frame=eg4.frame)
sc = extrusion(shape=shapes.rectangle(width=R6+R5/4, height=R5)-shapes.circle(radius=R2/3),
               pos=[(-3.4,0,R6), (-4.4,0,R6)], frame=eg6.frame, color=(0,0,1),
               material=materials.glass)
                                     
run = True
ang = pi/512
ang2 = ang/(R6/R5)
while True:
    if run:
        eg5.frame.rotate(axis=(0,0,1), angle=ang)
        eg6.frame.rotate(axis=(1,0,0), angle=ang2, origin=(0,0,R6))
    rate(100)
    if scene.mouse.events:
        m = scene.mouse.getevent()
        if m.click == 'left':
            run = not run
