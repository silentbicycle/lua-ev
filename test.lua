require "evc"

-- my.dump(evc)

print("Supported: ", evc.supported_backends())
print("Recommended: ", evc.recommended_backends())

local loop = evc.default_loop()
print("loop: ", loop)

local tim = loop:timer_init(2, 1)
print("timer: ", tim)

local ct = 3

print(loop, tim)
tim:set_cb(function (w, ev)
              print("YAY", w, ev)
              print(evc.time())
              ct = ct - 1
              if ct == 0 then tim:stop() end
           end)

print "set cb"
loop:timer_start(tim)
print "timer started"

print "about to loop"
loop:loop()

print "Done"
