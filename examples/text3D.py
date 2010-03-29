from visual import *

scene.height = 400
scene.width = 800
scene.range = 4
scene.autocenter = True
title = text(text='na mexa', align='center', depth=-.3, color=color.cyan)
height = curve(frame=title.frame, color=color.yellow, radius=0.02,
               pos=[title.start+vector(-0.2,0,0), title.start+vector(-0.2,title.height,0)])
lheight = label(frame=title.frame, pos=.5*(height.pos[0]+height.pos[1]),
                xoffset=-40, yoffset=+10, text='height')
descent = curve(frame=title.frame, color=color.yellow, radius=0.02,
               pos=[title.start+vector(0.2,0,0), title.start+vector(0.2,-title.descent,0)])
ldescent = label(frame=title.frame, pos=.5*(descent.pos[0]+descent.pos[1]),
                xoffset=30, yoffset=-20, text='descent')
width = curve(frame=title.frame, color=color.yellow, radius=0.02,
               pos=[title.upper_left+vector(0,0.1,0), title.upper_right+vector(0,0.1,0)])
lwidth = label(frame=title.frame, pos=.5*(width.pos[0]+width.pos[1]),
                xoffset=10, yoffset=40, text='width')
ul = label(frame=title.frame, pos=title.upper_left, text='upper_left',
           xoffset=-30, yoffset=30)
ur = label(frame=title.frame, pos=title.upper_right, text='upper_right',
           xoffset=30, yoffset=30)
lr = label(frame=title.frame, pos=title.lower_right, text='lower_right',
           xoffset=30, yoffset=-30)
ll = label(frame=title.frame, pos=title.lower_left, text='lower_left',
           xoffset=-30, yoffset=-30)
lc = label(frame=title.frame, text="pos (align='"+title.align+"')",
           xoffset=10, yoffset=-40)
ls = label(frame=title.frame, pos=title.start, text='start',
           xoffset=-60, yoffset=-30)
for s in [ul, ur, lr, ll, lc, ls]:
    sphere(frame=title.frame, pos=s.pos, radius=0.05, color=color.red)

