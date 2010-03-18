require "evc"

-- my.dump(evc)

print("Supported: ", evc.supported_backends())
print("Recommended: ", evc.recommended_backends())

local loop = evc.default_loop()

local tim = evc.timer_init(loop, 1)
print(loop, tim)
evc.set_cb(tim, function (w, ev) print("YAY", ev) end)
print "set cb"
evc.timer_start(loop, tim)
print "timer started"

print "about to loop"
evc.loop(loop)
