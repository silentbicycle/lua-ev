require "socket"
require "posix"

function stress_test()
   local ss = {}
   local ct = 120
   
   local i = 1
   while i < ct do
      local s = socket.connect("localhost", 12345)
      if s then
         s:settimeout(0)
         ss[i] = s
      end
      i = i + 1
   end
   
   while true do
      local i = math.random(1, ct)
      local sock = ss[i]
      if sock then
         local msg = (tostring(i) .. " "):rep(10)
         sock:send(msg)
--          print("SENT: ", msg)
         local resp = sock:receive(msg:len())
--          if resp then print("GOT: ", resp) end 
      end
   end
end

for proc=1,5 do
   if posix.fork() == 0 then stress_test() end
end


while true do
   socket.select(nil, nil, 1)
end
