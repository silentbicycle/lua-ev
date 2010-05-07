require "evc"

local loop = evc.default_loop()
print("loop: ", loop)

local tim = evc.timer_init(2, 0.5)
print("timer: ", tim)

local ct = 5

print(loop, tim)
tim:set_cb(function (w, ev)
              ct = ct - 1
              print("tick", w, evc.time())
              if ct == 0 then tim:stop(loop) end
              --for k,v in pairs(ev) do print("--> ", k,v ) end
              print ""
           end)
print "set cb"

tim:start(loop)
print "timer started"

print "about to loop"
loop:loop()

print "Done"
