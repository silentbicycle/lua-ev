require "evc"

local loop = evc.default_loop()
print("loop: ", loop)

local p = evc.periodic_init(2, 0.5)
print("periodic watcher: ", p)

local ct = 0
p:set_cb(function(w, ev)
            print("Tick", w, evc.time())
            ct = ct + 1
            if ct == 5 then p:stop(loop) end
         end)

p:start(loop)

print "starting loop"
loop:loop()

print "done"
