require "evc"

local loop = evc.default_loop()
print("loop: ", loop)

local tim = evc.timer_init(2, 0.5)
print("timer: ", tim)

local ct = 3

print(loop, tim)
tim:set_cb(function (w, ev)
              print("FUN", w, evc.time())
              ct = ct - 1
              if ct == 2 then tim:stop(loop) end
              for k,v in pairs(ev) do print("--> ", k,v ) end
              print ""
           end)
print "set cb"

tim:start(loop)
print "timer started"

print "about to loop"
loop:loop()

print "Done"
