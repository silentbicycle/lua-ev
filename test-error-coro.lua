require "evc"

local loop = evc.default_loop()
local tim = evc.timer_init(2, 0.5)

local ct = 3

local bar = coroutine.create(
   function ()
      local ct = 0
      while true do
         print("tick", ct)
         ct = ct + 1
         coroutine.yield("msg1", "msg2")
         if ct == 2 then error("crashola") end
      end
   end)


print(loop, tim)
tim:set_cb(bar)
print "set cb"

tim:start(loop)

print "about to loop"
loop:loop()

print "Done"
