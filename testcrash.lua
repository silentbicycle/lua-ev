require "crashy"

local function foo()
   print "foo"
   error("tomato")
   print "blaff"
end

local bar = coroutine.create(
   function ()
      local ct = 0
      while true do
         print("Ct is ", ct)
         ct = ct + 1
         coroutine.yield(ct)
         if ct == 2 then error("blaff") end
      end
   end)


print "-- function"
crashy.set(foo)
crashy.run()

print "\n-- coroutine"
crashy.set(bar)
crashy.run()
crashy.run()
crashy.run()
crashy.run()
