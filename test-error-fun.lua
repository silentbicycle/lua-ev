require "evc"

local loop = evc.default_loop()
local tim = evc.timer_init(2, 0.5)

local ct = 3

print(loop, tim)
tim:set_cb(function (w, ev)
              print("tick", w, evc.time())
              ct = ct - 1
              if ct == 0 then
                 print " -- Throwing error..."
                 error("the error message")
              end
           end)
print "set cb"

tim:start(loop)

print "about to loop"
loop:loop()

print "Done"
