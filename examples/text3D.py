from visual import *

scene.height = 400
scene.width = 800
scene.range = 4
scene.autocenter = True
title = text(text='My Text', align='center', depth=-.3, color=color.cyan)
vertical = (title.upper_left-title.start).norm()
height = curve(color=color.yellow, radius=0.02,
               pos=[title.start-.2*title.axis,
                    title.start-.2*title.axis+title.height*vertical])
lheight = label(pos=.5*(height.pos[0]+height.pos[1]),
                xoffset=-40, yoffset=+10, text='height')
descent = curve(color=color.yellow, radius=0.02,
               pos=[title.start+0.2*title.axis,
                    title.start+0.2*title.axis-title.descent*vertical])
ldescent = label(pos=.5*(descent.pos[0]+descent.pos[1]),
                xoffset=30, yoffset=-20, text='descent')
width = curve(color=color.yellow, radius=0.02,
               pos=[title.upper_left+0.1*vertical, title.upper_right+0.1*vertical])
lwidth = label(pos=.5*(width.pos[0]+width.pos[1]),
                xoffset=10, yoffset=40, text='width')
ul = label(pos=title.upper_left, text='upper_left',
           xoffset=-30, yoffset=30)
ur = label(pos=title.upper_right, text='upper_right',
           xoffset=30, yoffset=30)
lr = label(pos=title.lower_right, text='lower_right',
           xoffset=30, yoffset=-30)
ll = label(pos=title.lower_left, text='lower_left',
           xoffset=-30, yoffset=-30)
lc = label(text="pos (align='"+title.align+"')",
           xoffset=10, yoffset=-40)
ls = label(pos=title.start, text='start',
           xoffset=-60, yoffset=-30)
for s in [ul, ur, lr, ll, lc, ls]:
    sphere(pos=s.pos, radius=0.05, color=color.red)
