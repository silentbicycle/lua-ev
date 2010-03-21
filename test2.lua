require "evc"
require "socket"
require "tophat"

local con_ct, msg_ct, secs = 0, 0, 0
local job_queue = {}
local IDLE_WAIT = 0.01

local loop = evc.default_loop()
local subloop = evc.loop_new("kqueue")
local emb = evc.embed_init(subloop)
emb:start(loop)
local idle = evc.idle_init(loop)
idle:set_cb(function () run_job_queue(job_queue) end)
idle:start(loop)

local out_queue = {}

local function close(w, c)
   w:stop(subloop)
   out_queue[c] = nil
   c:close()
   con_ct = con_ct - 1
end


function run_job_queue(q)
   if next(q) then              --not empty
      local new_q = {}
      for job,info in pairs(q) do
         local ok, rest = coroutine.resume(job)
         local status = coroutine.status(job)
         if status == "suspended" then 
            new_q[job] = info
         elseif not ok then
            print("job died -> ", rest, status) -- TODO real error handling
         end
      end
      job_queue = new_q
   else
      socket.select(nil, nil, IDLE_WAIT) --don't busywait
   end
end


local function queue_send(iow, client, data)
   local q = out_queue[client] or {}
   table.insert(q, 1, data)
   out_queue[client] = q
   iow:stop(subloop)
   iow:set(client:getfd(), { read=true, write=true })
   iow:start(subloop)
   print("should now listen for write")
end


local function sendbuf(iow, client)
   if #q > 0 then
      local data = table.remove(out_queue, 1)
      print("TRYING TO SEND: ", data)
      local sent, err, partial = c:send(data)
      if not sent then
         if err == "closed" then close(w, c)
         elseif partial then
            queue_send(iow, client, data:sub(partial))
         end
      end
   end
end   


function spawn(coro_job, info)
   if type(coro_job) ~= "thread" then
      print "spawn argument must be coroutine"
      return
   end
   info = info or true
   job_queue[coro_job] = info
end


local function echoer(m)
   return coroutine.create(function()
                              print "in echoer"
                              coroutine.yield()
                              print(" --> GOT: ", m)
                           end)
end


local function listener(iow, c)
   local outbuf = {}
   return coroutine.create(
      function(w, ev)
         while true do
            print("pending? ", w:is_pending(), w:is_active())
            my.dump(ev)
            if ev.write then
               printf("GOT WRITE FLAG")
               sendbuf(iow, c)
            end

            if ev.read then
               local ok, err, rest = c:receive()
               print(ok, err, rest)
               local msg = ok or rest
               if msg ~= "" then 
                  print("queueing send", msg:upper())
                  queue_send(iow, c, msg:upper())
                  spawn(echoer(msg))
                  msg_ct = msg_ct + 1
               elseif err == "closed" then
                  c:close()
                  iow:stop(subloop)
                  return
               else
                  -- got an empty read, argh
                  iow:clear_pending(subloop)
                  iow:set(c:getfd(), 3)
                  iow:start(subloop)
               end
            end

            coroutine.yield()
         end
      end)
end


local function add_client(c)
   print("Got one: ", c)
   c:settimeout(0)
   con_ct = con_ct + 1
   local iow = evc.io_init(c:getfd(), { read=true })
   iow:set_cb(listener(iow, c))
   iow:start(subloop)
end


local function acceptor(iow, sock)
   return function(w, ev)
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
