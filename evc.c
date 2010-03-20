#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include <ev.h>

#include "evc.h"

/* TODO:
 * _ all watcher inits (esp. embed) and test
 * _ EV_MULTIPLICITY ?
 * _ flags (both as ints and tables)
 */

/*********************/
/* Utility functions */
/*********************/

static void do_error(lua_State *L, const char *e) {
        lua_pushstring(L, e); lua_error(L);
}


/* Do an async, unbuffered read.
 * The loop is just an arbitrary place to keep a buf...maybe use *L as an idx instead! */
static int async_read(lua_State *L) {
        check_ev_loop(L, 1);
        int fd = luaL_checkinteger(L, 2);
        lua_insert(L, 1);
        lua_gettable(L, LUA_REGISTRYINDEX);
        char *buf = (char *) lua_topointer(L, 3);
        if (buf == NULL) {
                lua_pushboolean(L, 0);
                printf("errno is %d\n", errno);
                lua_pushstring(L, "bad");
                return 2;
        }
        
        int ct = read(fd, buf, BUFSZ);
        if (ct == -1) {
                lua_pushboolean(L, 0);
                if (errno == EAGAIN) {
                        errno = 0;
                        lua_pushstring(L, "timeout");
                } else {
                        lua_pushstring(L, "error");
                }
                return 2;
        } else {
                lua_pushlstring(L, buf, ct);
                return 1;
        }
}


static int async_write(lua_State *L) {
        size_t len, written;
        int fd = luaL_checkinteger(L, 1);
        const char *buf = lua_tolstring(L, 2, &len);
        if (buf == NULL) do_error(L, "second argument must be string");
        written = write(fd, buf, len);
        if (written == -1) {
                lua_pushboolean(L, 0);
                if (errno == EAGAIN) {
                        errno = 0;
                        lua_pushstring(L, "timeout");
                } else {
                        lua_pushstring(L, "error");
                }
                return 2;
        } else {
                lua_pushnumber(L, written);
                return 1;
        }
}


/*****************************/
/* General library functions */
/*****************************/

pushnum(ev_version_major)
pushnum(ev_version_minor)
pushnum(ev_supported_backends)
pushnum(ev_recommended_backends)
pushnum(ev_embeddable_backends)
pushnum(ev_time)


static int lev_sleep(lua_State *L) {
        ev_tstamp ts = luaL_checknumber(L, 1);
        ev_sleep(ts);
        return 0;
}


/* ev_set_allocator (void *(*cb)(void *ptr, long size) -> void */
/* ev_set_syserr_cb (void *(*cb)(const char *msg) -> void */


/*********/
/* Loops */
/*********/

static int loop_tostring(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        struct ev_loop *t = loop->t;
        
        lua_pushfstring(L, "evloop: 0x%d", (long) t);
        return 1;
}

static int init_loop_mt(lua_State *L, Lev_loop *lev_loop, struct ev_loop *loop) {
        lev_loop->t = loop;
        ev_set_userdata(loop, L);  /* so event loop knows the Lua state for callbacks*/
        luaL_getmetatable(L, "Lev_loop");
        lua_setmetatable(L, -2);
        return 1;               /* leaving the loop */
}

static void register_loop_io_buffer(lua_State *L, struct ev_loop *loop) {
        char *buf = (char *)calloc(BUFSZ, sizeof(char *));
        lua_pushlightuserdata(L, loop);
        lua_pushlightuserdata(L, buf);
        lua_settable(L, LUA_REGISTRYINDEX);
}


static int lev_default_loop(lua_State *L) {
        int flags = luaL_optint(L, 1, 0);
        Lev_loop *lev_loop = (Lev_loop *)lua_newuserdata(L, sizeof(Lev_loop));
        struct ev_loop *loop = ev_default_loop(flags);
        register_loop_io_buffer(L, loop);
        return init_loop_mt(L, lev_loop, loop);
}

static int lev_loop_new(lua_State *L) {
        int flags = luaL_optint(L, 1, 0);
        Lev_loop *lev_loop = (Lev_loop *)lua_newuserdata(L, sizeof(Lev_loop));
        struct ev_loop *loop = ev_loop_new(flags);
        register_loop_io_buffer(L, loop);
        return init_loop_mt(L, lev_loop, loop);
}

static int lev_default_destroy(lua_State *L) {
        ev_default_destroy();
        return 0;
}

loop_fun_0(ev_loop_destroy)

static int lev_default_fork(lua_State *L) {
        ev_default_fork();
        return 0;
}

loop_fun_0(ev_loop_fork)

static int lev_is_default_loop(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        lua_pushboolean(L, ev_is_default_loop(loop->t));
        return 1;
}

loop_fun_n1(ev_loop_count)
loop_fun_n1(ev_loop_depth)
loop_fun_n1(ev_backend)
loop_fun_n1(ev_now)
loop_fun_0(ev_now_update)
loop_fun_0(ev_suspend)
loop_fun_0(ev_resume)

static int lev_loop(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        int flags = luaL_optint(L, 2, 0);
        ev_loop(loop->t, flags);
        return 0;
}

static int lev_unloop(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        int flags = luaL_checknumber(L, 2);
        ev_unloop(loop->t, flags);
        return 0;
}

loop_fun_0(ev_ref)
loop_fun_0(ev_unref)

static int lev_set_io_collect_interval(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_set_io_collect_interval(loop->t, luaL_checknumber(L, 2));
        return 0;
}

static int lev_set_timeout_collect_interval(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_set_timeout_collect_interval(loop->t, luaL_checknumber(L, 2));
        return 0;
}

loop_fun_0(ev_invoke_pending)
loop_fun_n1(ev_pending_count)
/* void ev_set_invoke_pending_cb (EV_P_ void (*invoke_pending_cb)(EV_P)); */
/* void ev_set_loop_release_cb (EV_P_ void (*release)(EV_P), void (*acquire)(EV_P)); */

static int lev_set_userdata(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        void *data = (void *)lua_topointer(L, 2);
        ev_set_userdata(loop->t, data);
        return 0;
}

static int lev_userdata(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        void *data = ev_userdata(loop->t);
        lua_pushlightuserdata(L, data);
        return 1;
}

loop_fun_0(ev_loop_verify)


/*****************************/
/* Generic watcher functions */
/*****************************/

watcher_bool(ev_is_active)
watcher_bool(ev_is_pending)

static const char *bad_watcher = "First argument is not a valid watcher";

/* Check for any kind of watcher. */
static Lev_watcher* check_watcher(lua_State *L, int n) {
        void *udata = lua_touserdata(L, n);
        if (udata == NULL) return NULL;
        lua_getmetatable(L, 1);
        if (lua_isnil(L, -1)) return NULL;
        lua_getfield(L, -1, "__watcher");
        lua_pushboolean(L, 1);
        if (!lua_equal(L, -1, -2)) return NULL;
        lua_pop(L, 3);
        return (Lev_watcher*) udata;
}

/* Set a callback: [ ev_watcher *w, luafun or coro ] */
static int lev_set_cb(lua_State *L) {
        Lev_watcher *watcher = check_watcher(L, 1);
        if (watcher == NULL) do_error(L, bad_watcher);
        int cbtype = lua_type(L, -1);
        if (cbtype == LUA_TFUNCTION || cbtype == LUA_TTHREAD) {
                ev_watcher *w = watcher->t;
                lua_pushlightuserdata(L, w);
                lua_insert(L, 2);
                lua_settable(L, LUA_REGISTRYINDEX);
        } else {
                do_error(L, "Second argument must be function or coroutine.");
        }
        return 0;
}

static int lev_set_priority(lua_State *L) {
        Lev_watcher *w = check_watcher(L, 1);
        if (w == NULL) do_error(L, bad_watcher);
        int prio = luaL_checknumber(L, 2);
        ev_set_priority((w->t), prio);
        return 0;
}

static int lev_priority(lua_State *L) {
        Lev_watcher *w = check_watcher(L, 1);
        if (w == NULL) do_error(L, bad_watcher);
        lua_pushnumber(L, ev_priority(w->t));
        return 1;
}


/******************************/
/* Specific watcher functions */
/******************************/

static int lev_io_init(lua_State *L) {
        int fd = luaL_checkinteger(L, 1);
        int flags = luaL_checkinteger(L, 2); /* R, W, R|W */
        lua_pop(L, 2);
        ALLOC_UDATA_AND_WATCHER(io);
        ev_io_init(io->t, (void *)call_luafun_cb, fd, flags);
        return 1;
}

static int lev_timer_init(lua_State *L) {
        double after = luaL_checknumber(L, 1);
        double repeat = luaL_optint(L, 2, 0);
        lua_pop(L, 2);
        ALLOC_UDATA_AND_WATCHER(timer);
        ev_timer_init(timer->t, (void *)call_luafun_cb, after, repeat);
        return 1;
}

static int lev_periodic_init(lua_State *L) {
        double offset = luaL_checknumber(L, 1);
        double interval = luaL_optnumber(L, 2, 0);
        /* ev_tstamp (*cb)(ev_periodic *, ev_tstamp): NYI */
        lua_pop(L, 2);
        ALLOC_UDATA_AND_WATCHER(periodic);
        ev_periodic_init(periodic->t, (void *)call_luafun_cb,
            offset, interval, 0);
        return 1;
}

static int lev_signal_init(lua_State *L) {
        int signum = luaL_checkinteger(L, 1);
        lua_pop(L, 1);
        ALLOC_UDATA_AND_WATCHER(signal);
        ev_signal_init(signal->t, (void *)call_luafun_cb, signum);
        return 1;
}

static int lev_child_init(lua_State *L) {
        int pid = luaL_checkinteger(L, 1);
        int trace = 0;
        if (lua_isboolean(L, 2)) {
                trace = lua_toboolean(L, 2);
                lua_pop(L, 1);
        }
        lua_pop(L, 1);
        ALLOC_UDATA_AND_WATCHER(child);
        ev_child_init(child->t, (void *)call_luafun_cb, pid, trace);
        return 1;
}

static int lev_stat_init(lua_State *L) {
        const char* path = luaL_checkstring(L, 1);
        double interval = 0;
        if (lua_isnumber(L, 2)) {
                interval = lua_tonumber(L, 2);
                lua_pop(L, 1);
        }
        lua_pop(L, 1);
        ALLOC_UDATA_AND_WATCHER(stat);
        ev_stat_init(stat->t, (void *)call_luafun_cb, path, interval);
        return 1;
}

static int lev_idle_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(idle);
        ev_idle_init(idle->t, (void *)call_luafun_cb);
        return 1;
}

static int lev_prepare_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(prepare);
        ev_prepare_init(prepare->t, (void *)call_luafun_cb);
        return 1;
}

static int lev_check_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(check);
        ev_check_init(check->t, (void *)call_luafun_cb);
        return 1;
}

static int lev_embed_init(lua_State *L) {
        Lev_loop *subloop = check_ev_loop(L, 1);
        ALLOC_UDATA_AND_WATCHER(embed);
        ev_embed_init(embed->t, (void *)call_luafun_cb, subloop->t);
        return 1;
}

static int lev_embed_sweep(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        Lev_embed *w = CHECK_WATCHER(2, embed);
        ev_embed_sweep(loop->t, w->t);
        return 0;
}

static int lev_fork_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(fork);
        ev_fork_init(fork->t, (void *)call_luafun_cb);
        return 1;
}

static int lev_async_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(async);
        ev_async_init(async->t, (void *)call_luafun_cb);
        return 1;
}

static int lev_async_send(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        Lev_async *w = CHECK_WATCHER(2, async);
        ev_async_send(loop->t, w->t);
        return 0;
}

static int lev_async_pending(lua_State *L) {
        Lev_async *w = CHECK_WATCHER(1, async);
        lua_pushboolean(L, ev_async_pending(w->t));
        return 1;
}

DEF_WATCHER_METHODS(io);
DEF_WATCHER_METHODS(timer);
DEF_WATCHER_METHODS(periodic);
DEF_WATCHER_METHODS(signal);
DEF_WATCHER_METHODS(child);
DEF_WATCHER_METHODS(stat);
DEF_WATCHER_METHODS(idle);
DEF_WATCHER_METHODS(prepare);
DEF_WATCHER_METHODS(check);
DEF_WATCHER_METHODS(embed);
DEF_WATCHER_METHODS(fork);
DEF_WATCHER_METHODS(async);

 
/*************/
/* Callbacks */
/*************/

static void call_luafun_cb(struct ev_loop *l, ev_watcher *w, int events) {
        lua_State *L = ev_userdata(l);
        lua_pushlightuserdata(L, w);
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (lua_isthread(L, -1)) {
                if (DEBUG) puts("About to resume coroutine\n");
                lua_State *Coro = (void *)lua_topointer(L, -1);
                lua_pushlightuserdata(Coro, w);
                lua_pushinteger(Coro, events);
                lua_resume(Coro, 2);
        } else if (lua_isfunction(L, -1)) {
                if (DEBUG) puts("About to call Lua callback fun\n");
                lua_pushlightuserdata(L, w);
                lua_pushinteger(L, events);
                lua_pcall(L, 2, 0, 0);
        } else {
                do_error(L, "Not a coroutine or function");
        }
}


/**********/
/* Module */
/**********/

static const struct luaL_Reg evlib[] = {
        { "time", lev_time },
        { "sleep", lev_sleep },
        { "version_major", lev_version_major },
        { "version_minor", lev_version_minor },
        { "supported_backends", lev_supported_backends },
        { "recommended_backends", lev_recommended_backends },
        { "embeddable_backends", lev_embeddable_backends },
        { "default_loop", lev_default_loop },
        { "ev_loop_new", lev_loop_new },
        { "ev_loop_count", lev_loop_count },
        { "set_cb", lev_set_cb },
        { "timer_init", lev_timer_init },
        { "io_init", lev_io_init },
        { "periodic_init", lev_periodic_init },
        { "signal_init", lev_signal_init },
        { "child_init", lev_child_init },
        { "stat_init", lev_stat_init },
        { "idle_init", lev_idle_init },
        { "prepare_init", lev_prepare_init },
        { "check_init", lev_check_init },
        { "embed_init", lev_embed_init },
        { "fork_init", lev_fork_init },
        { "async_init", lev_async_init },
        { NULL, NULL },
};

/* Where should these go? */
/* { "embed_sweep", lev_embed_sweep }, */
/* { "async_send", lev_async_send }, */
/* { "async_pending", lev_async_pending }, */


static const struct luaL_Reg loop_mt [] = {
        { "loop", lev_loop },
        { "unloop", lev_unloop },
        { "set_userdata", lev_set_userdata },
        { "userdata", lev_userdata },
        { "resume", lev_resume },
        { "suspend", lev_suspend },
        { "invoke_pending", lev_invoke_pending },
        { "pending_count", lev_pending_count },
        { "verify", lev_loop_verify },
        { "depth", lev_loop_depth },
        { "count", lev_loop_count },
        { "unref", lev_unref },
        { "ref", lev_ref },
        { "now", lev_now },
        { "now_update", lev_now_update },
        { "backend", lev_backend },
        { "default_fork", lev_default_fork },
        { "default_destroy", lev_default_destroy },
        { "destroy", lev_loop_destroy },
        { "fork", lev_loop_fork },
        { "is_default_loop", lev_is_default_loop },
        { "set_io_collect_interval", lev_set_io_collect_interval },
        { "set_timeout_collect_interval", lev_set_timeout_collect_interval },
        { "timer_start", lev_timer_start },
        { "io_start", lev_io_start },
        { "read", async_read },
        { "write", async_write },
        { NULL, NULL },
};

/* Set up a table with functions for each watcher. */
DEF_WATCHER_MT_VALS(io);
DEF_WATCHER_MT_VALS(timer);
DEF_WATCHER_MT_VALS(periodic);
DEF_WATCHER_MT_VALS(signal);
DEF_WATCHER_MT_VALS(child);
DEF_WATCHER_MT_VALS(stat);
DEF_WATCHER_MT_VALS(idle);
DEF_WATCHER_MT_VALS(prepare);
DEF_WATCHER_MT_VALS(check);
DEF_WATCHER_MT_VALS(embed);
DEF_WATCHER_MT_VALS(fork);
DEF_WATCHER_MT_VALS(async);


int luaopen_evc(lua_State *L) {
        luaL_newmetatable(L, "Lev_loop");
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, loop_tostring);
        lua_setfield(L, -2, "__tostring");
        luaL_register(L, NULL, loop_mt);

        /* Define each watcher metatable and put them in the registry */
        DEF_WATCHER_METATABLE(io);
        DEF_WATCHER_METATABLE(timer);
        DEF_WATCHER_METATABLE(periodic);
        DEF_WATCHER_METATABLE(signal);
        DEF_WATCHER_METATABLE(child);
        DEF_WATCHER_METATABLE(stat);
        DEF_WATCHER_METATABLE(idle);
        DEF_WATCHER_METATABLE(prepare);
        DEF_WATCHER_METATABLE(check);
        DEF_WATCHER_METATABLE(embed);
        DEF_WATCHER_METATABLE(fork);
        DEF_WATCHER_METATABLE(async);

        luaL_register(L, "evc", evlib);
        return 1;
}
