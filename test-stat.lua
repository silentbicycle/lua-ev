require "evc"

local loop = evc.default_loop()
print("loop: ", loop)

local stat = evc.stat_init("./foo")
print("stat (file change) watcher: ", stat)
print("-- Watching for changes to ./foo")

stat:set_cb(function(w, ev)
               print("* Changed")
               stat:stop(loop)
            end)

stat:start(loop)

print("stat's path is", stat:path())

print "starting loop"
loop:loop()

print "done"
