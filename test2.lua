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
local watchers = setmetatable({}, {__mode="kv"})

local function close(c)
   for _,w in ipairs(watchers[c] or {}) do w:stop(subloop) end
   out_queue[c] = nil
   c:close()
   con_ct = con_ct - 1
end


function run_job_queue(q)
   if next(q) then              --not empty
      local new_q = {}
      for job,info in pairs(q) do
         local jt = type(job)
         if jt == "function" then job()
         elseif jt == "thread" then
            print("rezoom")
            local ok, rest = coroutine.resume(job)
            local status = coroutine.status(job)
            if status == "suspended" then 
               new_q[job] = info
            elseif not ok then
               print("job died -> ", rest, status) -- TODO real error handling
            end
         end
      end
      job_queue = new_q
   else
      socket.select(nil, nil, IDLE_WAIT) --don't busywait
   end
end


local function writer(c)
   assert(c, "no client")
   return coroutine.create(
      function(w, ev)
         while true do
            print "writer"
            if ev.write then
               print("sendbuf? ", sendbuf)
               if sendbuf(c) then
                  print " -----> finishing"
                  return
               end
            end
            w, ev = coroutine.yield()
            print "whirr"
         end
         print "*blaff"
      end)
end


local function queue_send(client, data)
   local q = out_queue[client] or {}
   table.insert(q, 1, data)
   out_queue[client] = q
   print(#out_queue[client])
   local wr = assert(evc.io_init(client:getfd(), { write=true }))
   wr:set_cb(writer(client))
   wr:start(subloop)

   local ws = watchers[client] or {}
   ws[#ws+1] = wr
   watchers[client] = ws
   print("should now listen for write")
end


function sendbuf(client)
   assert(client)
   local q = out_queue[client] or {}
   if #q > 0 then
      local data = table.remove(q, 1)
      print("Trying to send: ", data, client)
      local sent, err, partial = client:send(data)
      print("SEP", sent, err, partial)
      if not sent then
         if err == "closed" then close(client)
         elseif partial then
            queue_send(client, data:sub(partial))
         end
      end
   else
      return true               --done
   end
end   


function spawn(job, info)
   local jt = type(job)
   if jt == "thread" or jt == "function" then
      job_queue[job] = info or true
   else
      print "spawn argument must be function or coroutine"
   end
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
            print "listener"
            if ev.read then
               local ok, err, rest = c:receive()
               local msg = ok or rest
               if msg ~= "" then 
                  print("queueing send", msg:upper())
                  queue_send(c, msg:upper())
                  spawn(echoer(msg), "echoer")
                  msg_ct = msg_ct + 1
               elseif err == "closed" then
                  close(c)
                  return
               elseif err == "timeout" then
                  -- nop
               else
                  print "WTF????"
               end
            end

            w, ev = coroutine.yield()
         end
      end)
end


local function add_client(c)
   print("Got one: ", c)
   c:settimeout(0)
   con_ct = con_ct + 1
   local rw = evc.io_init(c:getfd(), { read=true })
   rw:set_cb(listener(rw, c))
   rw:start(subloop)
   local ws = watchers[client] or {}
   ws[#ws+1] = rw
   watchers[c] = ws
end


local function acceptor(iow, sock)
   return function(w, ev)
             local client, err = sock:accept()
             while client do
                add_client(client)
                client, err = sock:accept()
             end
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

   local iow = evc.io_init(s:getfd(), "r" )
   iow:set_cb(acceptor(iow, s))
   iow:start(subloop)
   print("listening on port ", port)
   loop:loop()
end


main()
