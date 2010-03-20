require "evc"

-- my.dump(evc)

print("Supported: ", evc.supported_backends())
print("Recommended: ", evc.recommended_backends())

local loop = evc.default_loop()
print("loop: ", loop)

local tim = evc.timer_init(2)
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

local tim2 = evc.timer_init(1, 1)
local coro = coroutine.create(function (w, ev)
                    local ct = 0
                    while true do
                       print("In coro, ct=", ct, w, ev)
                       ct = ct + 1
                       coroutine.yield()
                    end
                 end)

tim2:set_cb(coro)
loop:timer_start(tim2)

print "about to init io watcher"
local iow = evc.io_init(0, 1) --stdin, read
assert(iow)
local iow_mt = getmetatable(iow)
iow_mt.__watcher = true

print "setting io cb"
iow:set_cb(function (w, ev)
                print("STDIN is Readable")
                local data, err = loop:read(0)
                print(data, err)
                iow:stop(loop)
             end)
print "set io cb"

loop:io_start(iow)

print "about to loop"
loop:loop()

print "Done"
