require "evc"
require "socket"

local con_ct, msg_ct, secs = 0, 0, 0

local loop = evc.default_loop()
local subloop = evc.loop_new("kqueue")
local emb = evc.embed_init(subloop)
emb:start(loop)
-- emb:set_cb(function()
--               emb:sweep(subloop)
--               emb:stop(loop)
--               emb:start(loop)
--            end)


local function spawn_coro(iow, c)
--    print("spawn coro")
   return coroutine.create(
      function(w, ev)
         while true do
            local ok, err, rest = c:receive()
            local msg = ok or rest
            if msg ~= "" then 
--                print("GOT: ", msg)
               c:send(msg)
               msg_ct = msg_ct + 1
            elseif err == "closed" then
               c:close()
               iow:stop(loop)
               return
            end
            coroutine.yield()
         end
      end)
end


local function add_client(c)
   c:settimeout(0)
   con_ct = con_ct + 1
   local iow = evc.io_init(c:getfd(), 1)
   iow:set_cb(spawn_coro(iow, c))
   iow:start(subloop)
--    iow:start(loop)
end


local function acceptor(iow, sock)
--    print "acceptor"
   return function(w, ev)
--              print(w, ev, iow, sock)
             local client, err = sock:accept()
             while client do
                add_client(client)
                client, err = sock:accept()
             end
             iow:stop(subloop)
             iow:start(subloop)
          end
end


function main(port)
   port = port or 12345

   local s = assert(socket.bind("*", port))
   s:setoption("tcp-nodelay", true)
   s:settimeout(0)
   
   local tim = evc.timer_init(1, 1)
   tim:set_cb(function()
                 secs = secs + 1
                 print(string.format("%d %d %d -> %f",
                       con_ct, msg_ct, secs, msg_ct / secs))
              end)
   tim:start(loop)

   local iow = evc.io_init(s:getfd(), 1) --{ read=true }
   iow:set_cb(acceptor(iow, s))
   iow:start(subloop)
   print("listening on port ", port)
   loop:loop()
end


main()
