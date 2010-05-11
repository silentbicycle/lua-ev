require "evc"
require "posix"

local loop = evc.default_loop()
print("loop: ", loop)

local pid = posix.fork()
if pid == 0 then                --in child
   for ct = 1,3 do
      print("in child...", ct)
      os.execute("sleep 1")
   end
   os.exit(1)
end


local chi = evc.child_init(pid)
print("child watcher: ", chi, "pid: ", pid)

local ct = 0
chi:set_cb(function(w, ev)
            print("Child exited", w, evc.time())
            chi:stop(loop)
         end)

chi:start(loop)

print "starting loop, waiting for child to terminate"
loop:loop()

print "done"
