require "evc"

local loop = evc.default_loop()
print("loop: ", loop)

local eric = evc.idle_init()
print("idle watcher: ", eric)

local ct = 0
eric:set_cb(function(w, ev)
               print("Idle", ct)
               ct = ct + 1
               if ct == 5 then eric:stop(loop) end
            end)

eric:start(loop)

print "starting loop"
loop:loop()

print "done"
