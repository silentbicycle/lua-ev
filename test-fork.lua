require "evc"
require "posix"

local loop = evc.default_loop()
print("loop: ", loop)

local pid

local fork = evc.fork_init()
print("fork watcher: ", fork)

fork:set_cb(function(w, ev)
               print("* in fork-watcher callback")
               fork:stop(loop)
            end)
fork:start(loop)

local tim = evc.timer_init(2)
tim:set_cb(function(w, ev)
              print "timer CB -> about to fork"
              pid = posix.fork()
              if pid == 0 then
                 print("in child")
                 print(loop:fork())
                 tim:stop(loop)
              else
                 print("in parent of child with pid ", pid)
                 print(loop:fork())
                 tim:stop(loop)
              end
           end)
tim:start(loop)

print "starting loop"
loop:loop()

print("done", pid)
