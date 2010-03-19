require "evc"

-- my.dump(evc)

print("Supported: ", evc.supported_backends())
print("Recommended: ", evc.recommended_backends())

local loop = evc.default_loop()
print("loop: ", loop)

local tim = loop:timer_init(2)
print("timer: ", tim)

print(loop, tim)
tim:set_cb(function (w, ev)
              print("YAY", w, ev)
           end)

print "set cb"
loop:timer_start(tim)
print "timer started"

print "about to loop"
loop:loop()

print "Done"
