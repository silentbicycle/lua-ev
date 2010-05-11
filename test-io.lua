require "evc"

local loop = evc.default_loop()
print("loop: ", loop)

local tim = evc.timer_init(2, 0.5)
print("timer: ", tim)

local emb_loop = evc.loop_new("kqueue")
print("emb_loop", emb_loop)
local emb = evc.embed_init(emb_loop)
print("emb", emb)

print "about to init io watcher"
local iow = evc.io_init(0, "r") --stdin, read
assert(iow)

print "setting io cb"
iow:set_cb(function (w, ev)
                print("STDIN is Readable")
                local data, err = emb_loop:read(0) -- 0->stdin
                print("GOT", data, err)
                iow:stop(loop)
             end)
print "set io cb"

iow:start(emb_loop)
emb:start(loop)

print("io watcher's file descriptor is ", iow:fd())

print "about to loop, type something..."
loop:loop()

print "Done"
