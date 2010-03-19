require "evc"

-- my.dump(evc)

print("Supported: ", evc.supported_backends())
print("Recommended: ", evc.recommended_backends())

local loop = evc.default_loop()
print("loop: ", loop)

local tim = evc.timer_init(2, 3)
print("timer: ", tim)

local tim_mt = getmetatable(tim)
-- TODO fix this
tim_mt.__watcher = true

local ct = 3

print(loop, tim)
tim:set_cb(function (w, ev)
              print("YAY", w, ev)
              print(evc.time())
              ct = ct - 1
              if ct == 0 then tim:stop() end
           end)
print "set cb"

loop:timer_start(tim)           --or tim:start(loop) ?
print "timer started"

print "about to init io watcher"
local steve = evc.io_init(0, 1) --stdin, read
assert(steve)
local steve_mt = getmetatable(steve)
steve_mt.__watcher = true

print "setting io cb"
steve:set_cb(function (w, ev)
                print("STDIN is Readable")
             end)
print "set io cb"

loop:io_start(steve)

print "about to loop"
loop:loop()

print "Done"
