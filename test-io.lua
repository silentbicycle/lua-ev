require "evc"

local loop = evc.default_loop()

-- Typically, you want to run IO watchers in an embedded loop, since
-- backends such as epoll (on Linux) and kqueue (on BSD) scale better
-- than the defaults (select, poll), but they are only fully supported
-- with IO watchers.
local sups = evc.supported_backends()
local embs = evc.embeddable_backends()
local emb_backend
if sups.kqueue and embs.kqueue then emb_backend = "kqueue"
elseif sups.epoll and embs.epoll then emb_backend = "epoll"
else emb_backend = nil          --just use default
end

local emb_loop = evc.loop_new(emb_backend)
print("emb_loop", emb_loop)
local emb = evc.embed_init(emb_loop)
print("emb", emb)

print "-- about to init io watcher"
local iow = evc.io_init(0, "r") --stdin, read
assert(iow, "io watcher init failed")

print "-- setting io cb"
iow:set_cb(function (w, ev)
                print("STDIN is Readable")
                local data, err = iow:read()
                print(string.format("GOT: %q, error=%s", data, tostring(err)))
                iow:stop(loop)
             end)

print "-- starting subloop and its io watcher"
iow:start(emb_loop)
emb:start(loop)

print("io watcher's file descriptor is ", iow:fd())

print "-- about to loop, type something..."
loop:loop()

print "Done"
