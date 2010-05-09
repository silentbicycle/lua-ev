require "evc"

local loop = evc.default_loop()
print("loop: ", loop)

local SIGHUP = 1
local sig = evc.signal_init(SIGHUP)
print("signal watcher: ", sig)

sig:set_cb(function(w, ev)
              print("Got HUP")
              sig:stop(loop)
           end)

sig:start(loop)

print "starting loop...(send this process a signal)"
loop:loop()

print "done"
