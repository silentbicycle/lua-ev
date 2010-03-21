require "evc"

print("Supported: ")
for k,v in pairs(evc.supported_backends()) do print("", k,v) end
print("Recommended: ")
for k,v in pairs(evc.recommended_backends()) do print("", k,v) end
print("Embeddable: ")
for k,v in pairs(evc.embeddable_backends()) do print("", k,v) end

local loop = evc.default_loop()
print("loop: ", loop)

local tim = evc.timer_init(2)
print("timer: ", tim)

local emb_loop = evc.loop_new("kqueue")
print("emb_loop", emb_loop)
local emb = evc.embed_init(emb_loop)
print("emb", emb)

-- TODO fix this

-- for _,flags in ipairs{ 1, 3, 128, 256, 4096, 65536 } do
--    print(" -- flags -> table ", flags)
--    for k,v in pairs(evc.flags_to_table(flags)) do
--       print(k, v)
--    end   
-- end

local ct = 3

print(loop, tim)
tim:set_cb(function (w, ev)
              print("YAY", w, ev)
              print(evc.time())
              ct = ct - 1
              if ct == 0 then tim:stop() end
              for k,v in pairs(ev) do print("--> ", k,v ) end
           end)
print "set cb"

tim:start(loop)
print "timer started"

local tim2 = evc.timer_init(1, 1)
local coro = coroutine.create(function (w, ev)
                    local ct = 0
                    while true do
                       print("In coro, ct=", ct, w)
                       printf("Flags: ")
                       for k,v in pairs(ev) do print(k,v) end
                       ct = ct + 1
                       coroutine.yield()
                    end
                 end)

tim2:set_cb(coro)
tim2:start(loop)

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
