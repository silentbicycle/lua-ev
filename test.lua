require "evc"

local loop = evc.default_loop()
print("loop: ", loop)

local tim = evc.timer_init(2, 0.5)
print("timer: ", tim)

local emb_loop = evc.loop_new("kqueue")
print("emb_loop", emb_loop)
local emb = evc.embed_init(emb_loop)
print("emb", emb)

local ct = 3

print(loop, tim)
tim:set_cb(function (w, ev)
              print("FUN", w, evc.time())
              ct = ct - 1
              if ct == 2 then
                 print "Throwing error"
                 error("wocka wocka wocka")
                 print("resuming?")
                 tim:stop(loop)
              end
              for k,v in pairs(ev) do print("--> ", k,v ) end
              print ""
           end)
print "set cb"

tim:start(loop)
print "timer started"

local tim2 = evc.timer_init(1, 1)
local coro = coroutine.create(function (w, ev)
                    local ct = 0
                    while true do
                       print("CORO, ct=", ct, w, "flags:")
                       for k,v in pairs(ev) do print("--> ", k,v) end
                       print ""
                       if ct == 3 then
                          print("********** About to crash **********")
                          error("crasharooney")
                       end
                       ct = ct + 1
                       coroutine.yield()
                    end
                 end)

-- tim2:set_cb(coro)
-- tim2:start(loop)

print "about to init io watcher"
local iow = evc.io_init(0, 1) --stdin, read
assert(iow)

print "setting io cb"
iow:set_cb(function (w, ev)
                print("STDIN is Readable")
                local data, err = loop:read(0)
                print(data, err)
                iow:stop(loop)
             end)
print "set io cb"

iow:start(loop)

print "about to loop"
loop:loop()

print "Done"
