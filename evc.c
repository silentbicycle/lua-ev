#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include <ev.h>
#include <assert.h>
#include <string.h>

#include "evc.h"


/* TODO:
 * _ test all watcher inits
 * x flags (both as ints and tables)
 * _ EV_MULTIPLICITY ?
 */

/*********************/
/* Utility functions */
/*********************/

static const char* l_typename(int t) {
        switch (t) {
        case LUA_TNIL: return "nil";
        case LUA_TBOOLEAN: return "boolean";
        case LUA_TLIGHTUSERDATA: return "lightuserdata";
        case LUA_TNUMBER: return "number";
        case LUA_TSTRING: return "string";
        case LUA_TTABLE: return "table";
        case LUA_TFUNCTION: return "function";
        case LUA_TUSERDATA: return "userdata";
        case LUA_TTHREAD: return "thread";
        default: return "matchfail";
        }
}

/* Print stack, for debugging. */
static void pr_stack(lua_State *L) {
        int ct = lua_gettop(L);
        int i;
        printf("----------\nStack height is %d\n", ct);
        for (i=1; i <= ct; i++)
                printf("    %d: %s\t%s %lx\n",
                    i, l_typename(lua_type(L, i)),
                    lua_tostring(L, i), (long)lua_topointer(L, i));
        puts("----------");
}

static void do_error(lua_State *L, const char *e) {
        lua_pushstring(L, e); lua_error(L);
}

static const char *loop_buf_key = "evc_loopbuf";

/* Do an async, unbuffered read. */
static int async_read(lua_State *L) {
        Lev_io *w = CHECK_WATCHER(1, io);
        int fd = luaL_optint(L, 2, w->t->fd);
        char *buf;
        long ct;
        lua_pushstring(L, loop_buf_key);
        lua_gettable(L, LUA_REGISTRYINDEX);
        buf = (char *) lua_topointer(L, -1);
        if (buf == NULL) {
                lua_pushboolean(L, 0);
                printf("errno is %d\n", errno);
                lua_pushstring(L, "bad");
                return 2;
        }
        
        ct = read(fd, buf, BUFSZ);
        if (ct == -1) {
                lua_pushboolean(L, 0);
                if (errno == EAGAIN) {
                        puts("D");
                        errno = 0;
                        lua_pushstring(L, "timeout");
                } else {
                        lua_pushstring(L, "error");
                }
                return 2;
        } else {
                lua_pushlstring(L, buf, (unsigned long)ct);
                return 1;
        }
}

/* Do an async, unbuffered write. */
static int async_write(lua_State *L) {
        Lev_io *w = CHECK_WATCHER(1, io);
        size_t len;
        long written;
        int fd = luaL_optint(L, 2, w->t->fd);
        const char *buf = lua_tolstring(L, 3, &len);
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

/* int w/ bitflags -> table */
static int flags_to_table(lua_State *L, int flags) {
        unsigned int i;
        lua_newtable(L);

        for (i=1; i < EV_ERROR; i <<= 1) {
                switch (flags & i) {
                case EV_READ: setbitfield("read"); setbitfield("io"); break;
                case EV_WRITE: setbitfield("write"); break;
                case EV_TIMEOUT: setbitfield("timer"); break;
                case EV_PERIODIC: setbitfield("periodic"); break;
                case EV_SIGNAL: setbitfield("signal"); break;
                case EV_CHILD: setbitfield("child"); break;
                case EV_STAT: setbitfield("stat"); break;
                case EV_IDLE: setbitfield("idle"); break;
                case EV_PREPARE: setbitfield("prepare"); break;
                case EV_CHECK: setbitfield("check"); break;
                case EV_EMBED: setbitfield("embed"); break;
                case EV_FORK: setbitfield("fork"); break;
                case EV_ASYNC: setbitfield("async"); break;
                case EV_CUSTOM: setbitfield("custom"); break;
                case EV_ERROR: setbitfield("error"); break;
                default: ; /* there are gaps */
                }
        }
        return 1;
}

static unsigned int getflag(lua_State *L, const char *tag) {
        if (strcmp(tag, "async") == 0) return EV_ASYNC;
        if (strcmp(tag, "check") == 0) return EV_CHECK;
        if (strcmp(tag, "child") == 0) return EV_CHILD;
        if (strcmp(tag, "custom") == 0) return EV_CUSTOM;
        if (strcmp(tag, "embed") == 0) return EV_EMBED;
        if (strcmp(tag, "error") == 0) return EV_ERROR;
        if (strcmp(tag, "fork") == 0) return EV_FORK;
        if (strcmp(tag, "idle") == 0) return EV_IDLE;
        if (strcmp(tag, "periodic") == 0) return EV_PERIODIC;
        if (strcmp(tag, "prepare") == 0) return EV_PREPARE;
        if (strcmp(tag, "read") == 0) return EV_READ;
        if (strcmp(tag, "signal") == 0) return EV_SIGNAL;
        if (strcmp(tag, "stat") == 0) return EV_STAT;
        if (strcmp(tag, "timer") == 0) return EV_TIMER;
        if (strcmp(tag, "write") == 0) return EV_WRITE;
        do_error(L, "Bad tag");
        return -1;
}

/* table -> int w/ bitflags */
static unsigned int table_to_flags(lua_State *L, int idx) {
        const char* flagname;
        unsigned int flags = 0;
        if (!lua_istable(L, idx)) {
                lua_pushfstring(L, "Table or expected at argument #%d\n", idx);
                lua_error(L);
        }
        lua_pushnil(L);
        while (lua_next(L, idx)) { /* stack: [-2=k, -1=v] */
                lua_pop(L, 1);    /* we don't need the val */
                flagname = luaL_checkstring(L, -1);
                flags |= getflag(L, flagname);
        }
        return flags;
}

static const char* backend_to_name(unsigned int be) {
        const char* name;
        switch (be) {
        case EVFLAG_AUTO: name = "auto"; break;
        case EVBACKEND_SELECT: name = "select"; break;
        case EVBACKEND_POLL: name = "poll"; break;
        case EVBACKEND_EPOLL: name = "epoll"; break;
        case EVBACKEND_KQUEUE: name = "kqueue"; break;
        case EVBACKEND_DEVPOLL: name = "devpoll"; break;
        case EVBACKEND_PORT: name = "port"; break;
        default: name = "unmatched";
        }
        return name;
}

static int backends_to_table(lua_State *L, int bes) {
        uint i;
        lua_newtable(L);

        for (i=1; i <= EVBACKEND_PORT; i <<= 1) {
                switch (bes & i) {
                case EVBACKEND_SELECT: setbitfield("select"); break;
                case EVBACKEND_POLL: setbitfield("poll"); break;
                case EVBACKEND_EPOLL: setbitfield("epoll"); break;
                case EVBACKEND_KQUEUE: setbitfield("kqueue"); break;
                case EVBACKEND_DEVPOLL: setbitfield("devpoll"); break;
                case EVBACKEND_PORT: setbitfield("port"); break;
                default: ;
                }
        }
        return 1;
}

static int name_to_backend(lua_State *L, const char* tag) {
        if (strcmp(tag, "devpoll") == 0) return EVBACKEND_DEVPOLL;
        if (strcmp(tag, "epoll") == 0) return EVBACKEND_EPOLL;
        if (strcmp(tag, "kqueue") == 0) return EVBACKEND_KQUEUE;
        if (strcmp(tag, "poll") == 0) return EVBACKEND_POLL;
        if (strcmp(tag, "port") == 0) return EVBACKEND_PORT;
        if (strcmp(tag, "select") == 0) return EVBACKEND_SELECT;
        do_error(L, "Bad backend");
        return -1;
}


/*****************************/
/* General library functions */
/*****************************/

pushnum(ev_version_major)
pushnum(ev_version_minor)
push_backend_table(ev_supported_backends)
push_backend_table(ev_recommended_backends)
push_backend_table(ev_embeddable_backends)
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
        
        lua_pushfstring(L, "ev_loop(%s): %p",
            backend_to_name(ev_backend(loop->t)), t);
        return 1;
}

static int init_loop_mt(lua_State *L, Lev_loop *lev_loop, struct ev_loop *loop) {
        lev_loop->t = loop;
        ev_set_userdata(loop, L);  /* so event loop knows the Lua state for callbacks*/
        luaL_getmetatable(L, "Lev_loop");
        lua_setmetatable(L, -2);

        /* Save loop -> lev_loop in registry, so callbacks can get at it. */
        lua_pushlightuserdata(L, loop);
        lua_pushvalue(L, -2);
        lua_settable(L, LUA_REGISTRYINDEX);

        return 1;               /* leaving the loop */
}

static void register_loop_io_buffer(lua_State *L) {
        char *buf = (char *)calloc(BUFSZ, sizeof(char *));
        lua_pushstring(L, loop_buf_key);
        lua_pushlightuserdata(L, buf);
        lua_settable(L, LUA_REGISTRYINDEX);
}

static unsigned int get_flag_of_int_or_str(lua_State *L) {
        if (lua_gettop(L) == 0)
                return 0;
        else if (lua_isnumber(L, 1))
                return luaL_checkinteger(L, 1);
        else if (lua_isstring(L, 1)) {
                const char *name = lua_tostring(L, 1);
                int n = name_to_backend(L, name);
                if (n < 0) {
                        lua_pushfstring(L, "Unknown backend: %s", name);
                        lua_error(L);
                }
                return n;
        } else
                do_error(L, "Loop arg must be int(flag) or string.");
        return 0;
}

static int lev_default_loop(lua_State *L) {
        unsigned int flags = get_flag_of_int_or_str(L);
        Lev_loop *lev_loop = (Lev_loop *)lua_newuserdata(L, sizeof(Lev_loop));
        struct ev_loop *loop = ev_default_loop(flags);
        register_loop_io_buffer(L);
        return init_loop_mt(L, lev_loop, loop);
}

static int lev_loop_new(lua_State *L) {
        unsigned int flags = get_flag_of_int_or_str(L);
        Lev_loop *lev_loop = (Lev_loop *)lua_newuserdata(L, sizeof(Lev_loop));
        struct ev_loop *loop = ev_loop_new(flags);
        /* register ev_loop <-> lev_loop */
        register_loop_io_buffer(L);
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

static int lev_step(lua_State *L) { /* only do one event sweep */
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_loop(loop->t, EVLOOP_NONBLOCK);
        return 0;
}

static int lev_unloop(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        int flags = luaL_checkinteger(L, 2);
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

watcher_bool(ev_is_active)
watcher_bool(ev_is_pending)

/* Set a callback: [ ev_watcher *w, luafun or coro ] */
static int lev_set_cb(lua_State *L) {
        Lev_watcher *watcher = check_watcher(L, 1);
        int cbtype = lua_type(L, 2);
        if (watcher == NULL) do_error(L, bad_watcher);
        if (cbtype == LUA_TNIL || cbtype == LUA_TFUNCTION
            || cbtype == LUA_TTHREAD) {
                lua_settable(L, LUA_REGISTRYINDEX); /* reg[Lev_w] -> cb */
        } else {
                do_error(L, "Second argument must be function or coroutine.");
        }
        return 0;
}

static int lev_set_priority(lua_State *L) {
        Lev_watcher *w = check_watcher(L, 1);
        int prio = luaL_checkinteger(L, 2);
        if (w == NULL) do_error(L, bad_watcher);
        ev_set_priority(w->t, prio);
        return 0;
}

static int lev_priority(lua_State *L) {
        Lev_watcher *w = check_watcher(L, 1);
        if (w == NULL) do_error(L, bad_watcher);
        lua_pushnumber(L, ev_priority(w->t));
        return 1;
}


/* ev_invoke (loop, ev_TYPE *watcher, int revents) */
/* int ev_clear_pending (loop, ev_TYPE *watcher) */
/* ev_feed_event (loop, ev_TYPE *watcher, int revents) */
static int lev_clear_pending(lua_State *L) {
        Lev_watcher *w = check_watcher(L, 1);
        Lev_loop *loop = check_ev_loop(L, 2);
        ev_clear_pending(loop->t, w->t);
        return 0;
}


/******************************/
/* Specific watcher functions */
/******************************/

static int str_to_flags(lua_State *L, int idx) {
        int i, flags=0;
        const char *spec = lua_tostring(L, idx);
        for (i=0; i < 2; i++) {
                if (spec[i] == '\0') break;
                else if (spec[i] == 'r') flags |= EV_READ;
                else if (spec[i] == 'w') flags |= EV_WRITE;
        }
        return flags;
}

static int parse_flag_spec(lua_State *L, int idx) {
        int t = lua_type(L, idx);
        switch (t) {
        case LUA_TNUMBER:
                return luaL_checkinteger(L, idx);
        case LUA_TTABLE:
                return table_to_flags(L, idx);
        case LUA_TSTRING:       /* r, w, or rw */
                return str_to_flags(L, idx);
        default:
                do_error(L, "Flag int or table expected");
                return EV_ERROR;
        }
}

static int lev_io_init(lua_State *L) {
        unsigned int fd = luaL_checkinteger(L, 1);
        unsigned int flags = parse_flag_spec(L, 2);
        if (flags & EV_ERROR) do_error(L, "Bad flags");
        lua_pop(L, 2);
        ALLOC_UDATA_AND_WATCHER(io);

        ev_io_init(io->t, (void *)call_luafun_cb, fd, flags);
/*         printf(" -- Initiating IO w/ flags=%d for fd %d\n", flags, fd); */
        REGISTER_WATCHER(io);
        return 1;
}

static int lev_io_set(lua_State *L) {
        Lev_io *w = CHECK_WATCHER(1, io);
        int fd = luaL_checkinteger(L, 2);
        int flags = parse_flag_spec(L, 3);
        if (flags && EV_ERROR) do_error(L, "Bad flags");
/*         printf(" -- Setting IO flags to %d for fd %d\n", flags, fd); */
        if (ev_is_active(w->t)) printf("WARNING: setting active io watcher\n");
        ev_io_set(w->t, fd, flags);
        return 0;
}

static int lev_io_get_fd(lua_State *L) {
        Lev_io *w = CHECK_WATCHER(1, io);
        lua_pushinteger(L, w->t->fd);
        return 1;
}

static int lev_timer_init(lua_State *L) {
        double after = luaL_checknumber(L, 1);
        double repeat = luaL_optnumber(L, 2, 0);
        lua_pop(L, 2);
        ALLOC_UDATA_AND_WATCHER(timer);
        ev_timer_init(timer->t, (void *)call_luafun_cb, after, repeat);
        REGISTER_WATCHER(timer);
        return 1;
}

static int lev_timer_set(lua_State *L) {
        Lev_timer *w = CHECK_WATCHER(1, timer);
        double after = luaL_checknumber(L, 2);
        double repeat = luaL_optnumber(L, 3, 0);
        ev_timer_set(w->t, after, repeat);
        return 0;
}

static int lev_periodic_init(lua_State *L) {
        double offset = luaL_checknumber(L, 1);
        double interval = luaL_optnumber(L, 2, 0);
        /* ev_tstamp (*cb)(ev_periodic *, ev_tstamp): NYI */
        lua_pop(L, 2);
        ALLOC_UDATA_AND_WATCHER(periodic);
        ev_periodic_init(periodic->t, (void *)call_luafun_cb,
            offset, interval, 0);
        REGISTER_WATCHER(periodic);
        return 1;
}

static int lev_periodic_set(lua_State *L) {
        Lev_periodic *w = CHECK_WATCHER(1, periodic);
        double offset = luaL_checknumber(L, 2);
        double interval = luaL_optnumber(L, 3, 0);
        ev_periodic_set(w->t, offset, interval, 0);
        return 0;
}

static int lev_signal_init(lua_State *L) {
        /* TODO - map signal names ("HUP", etc.) to numbers. */
        int signum = luaL_checkinteger(L, 1);
        lua_pop(L, 1);
        ALLOC_UDATA_AND_WATCHER(signal);
        ev_signal_init(signal->t, (void *)call_luafun_cb, signum);
        REGISTER_WATCHER(signal);
        return 1;
}

static int lev_signal_set(lua_State *L) {
        Lev_signal *w = CHECK_WATCHER(1, signal);
        int signum = luaL_checknumber(L, 2);
        ev_signal_set(w->t, signum);
        return 0;
}

static int lev_signal_get_signum(lua_State *L) {
        Lev_signal *w = CHECK_WATCHER(1, signal);
        lua_pushinteger(L, w->t->signum);
        return 1;
}

static int lev_child_init(lua_State *L) {
        int pid = luaL_optnumber(L, 1, 0);
        int trace = 0;
        if (lua_isboolean(L, 2)) {
                trace = lua_toboolean(L, 2);
                lua_pop(L, 1);
        }
        lua_pop(L, 1);
        ALLOC_UDATA_AND_WATCHER(child);
        ev_child_init(child->t, (void *)call_luafun_cb, pid, trace);
        REGISTER_WATCHER(child);
        return 1;
}

static int lev_child_set(lua_State *L) {
        Lev_child *w = CHECK_WATCHER(1, child);
        int pid = luaL_checknumber(L, 2);
        int trace = 0;
        if (lua_isboolean(L, 2)) {
                trace = lua_toboolean(L, 2);
                lua_pop(L, 1);
        }
        ev_child_set(w->t, pid, trace);
        return 0;
}

static int lev_child_get_pid(lua_State *L) {
        Lev_child *w = CHECK_WATCHER(1, child);
        lua_pushinteger(L, w->t->pid);
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
        REGISTER_WATCHER(stat);
        return 1;
}

static int lev_stat_set(lua_State *L) {
        Lev_stat *w = CHECK_WATCHER(1, stat);
        const char* path = luaL_checkstring(L, 1);
        double interval = 0;
        if (lua_isnumber(L, 2)) {
                interval = lua_tonumber(L, 2);
                lua_pop(L, 1);
        }
        ev_stat_set(w->t, path, interval);
        return 0;
}

static int lev_stat_get_path(lua_State *L) {
        Lev_stat *w = CHECK_WATCHER(1, stat);
        lua_pushstring(L, w->t->path);
        return 1;
}

static int lev_idle_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(idle);
        ev_idle_init(idle->t, (void *)call_luafun_cb);
        REGISTER_WATCHER(idle);
        return 1;
}

static int lev_prepare_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(prepare);
        ev_prepare_init(prepare->t, (void *)call_luafun_cb);
        REGISTER_WATCHER(prepare);
        return 1;
}

static int lev_check_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(check);
        ev_check_init(check->t, (void *)call_luafun_cb);
        REGISTER_WATCHER(check);
        return 1;
}

static int lev_embed_init(lua_State *L) {
        Lev_loop *subloop = check_ev_loop(L, 1);
        ALLOC_UDATA_AND_WATCHER(embed);
        /* A NULL callback does the right thing for most cases. TODO. */
        ev_embed_init(embed->t, /*(void *)call_luafun_cb*/ 0, subloop->t);
        REGISTER_WATCHER(embed);
        return 1;
}

static int lev_embed_set(lua_State *L) {
        Lev_embed *w = CHECK_WATCHER(1, embed);
        Lev_loop *subloop = check_ev_loop(L, 1);
        ev_embed_set(w->t, subloop->t);
        return 0;
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
        REGISTER_WATCHER(fork);
        return 1;
}

static int lev_async_init(lua_State *L) {
        ALLOC_UDATA_AND_WATCHER(async);
        ev_async_init(async->t, (void *)call_luafun_cb);
        REGISTER_WATCHER(async);
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

static const char *err_handler_tag = "lev_error_handler";
static int def_error_handler(lua_State *L) {
        const char *e = lua_tostring(L, 1);
        printf(" *** In error handler (ht:%d) ------> %s\n",
            lua_gettop(L), e);
        pr_stack(L);
        return 1;
}


static void call_luafun_cb(struct ev_loop *l, ev_watcher *w, int events) {
        lua_State *L = ev_userdata(l);
        if (L == NULL) { printf("null L\n"); return; }

        if (0) printf("TOP %d %d %pl %s\n", lua_gettop(L),
            lua_type(L, -1), lua_topointer(L, -1), lua_tostring(L, -1));
        lua_pop(L, lua_gettop(L));          /* why? */

        lua_pushlightuserdata(L, w);        /* [ev_w] */
        lua_gettable(L, LUA_REGISTRYINDEX); /* [Lev_w] */
        lua_pushstring(L, err_handler_tag); /* [Lev_w "errtag"] */
        lua_gettable(L, LUA_REGISTRYINDEX); /* [Lev_w err_cb] */
        lua_insert(L, -2);                  /* [err_cb Lev_w] */
        lua_pushvalue(L, -1);               /* [err_cb Lev_w Lev_w] */
        lua_gettable(L, LUA_REGISTRYINDEX); /* [err_cb Lev_w cb] */
        lua_insert(L, -2);                  /* [err_cb cb Lev_w] */
        if (DEBUG) printf("top=%d, events=%d, cbtype=%d, "
                          "fun/coro=0x%lx, Lev_w(%d)=0x%lx\n",
            lua_gettop(L), events, lua_type(L, -2), (long)lua_topointer(L, -2),
            lua_type(L, -1), (long)lua_topointer(L, -1));

        int res=0;
        if (lua_isthread(L, -2)) {
                lua_State *Coro = (void *)lua_topointer(L, -2);
                lua_xmove(L, Coro, 1);        /* pass Lev_w to coro */
                flags_to_table(Coro, events); /* [cb, Lev_w, event_t] */
                res = lua_resume(Coro, 2);
                if (res > LUA_YIELD) {
                        /* coroutine crashed but stack is still here, get info from it */
                        def_error_handler(Coro);
                }
                lua_pop(L, 1);
        } else if (lua_isfunction(L, -2)) {
                /* TODO Look up error handler here, not for coros too */

                flags_to_table(L, events);  /* [err_cb, cb, Lev_w, event_t] */
                if (DEBUG) { puts("About to call Lua callback fun"); pr_stack(L); }
                res = lua_pcall(L, 2, 0, -4);
                if (DEBUG) {printf("After callback fun: %d\n", res); pr_stack(L); }
        } else {
                if (DEBUG) puts("No callback defined...clearing.");
#define DIDNT_HAVE_CALLBACK 8
                res = DIDNT_HAVE_CALLBACK;
        }
        if (DEBUG) { puts("after"); pr_stack(L); }
        if (res > LUA_YIELD) {  /* On error, stop watcher + clear callback */
                if (DEBUG) { printf("*** crashed, clearing callback, %d\n", res); pr_stack(L); }
                lua_pushlightuserdata(L, w);
                lua_gettable(L, LUA_REGISTRYINDEX);
                lua_pushstring(L, "stop");
                lua_gettable(L, -2);
                lua_pushvalue(L, -2); /* [lev_w.stop, lev_w] */

                lua_pushlightuserdata(L, l); /* [lev_w.stop, lev_w, ev_loop] */
                lua_gettable(L, LUA_REGISTRYINDEX); /* [lev_w.stop, lev_w, lev_loop] */
                lua_call(L, 2, 0); /* lev_w.stop(lev_w, l) */

                if (DEBUG) puts("Clearing watcher -> lev_watcher from registry");
                lua_pushlightuserdata(L, w);
                lua_pushnil(L);
                lua_settable(L, LUA_REGISTRYINDEX);
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
        { "loop_new", lev_loop_new },
        { "loop_count", lev_loop_count },
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

static const struct luaL_Reg loop_mt [] = {
        { "loop", lev_loop },
        { "step", lev_step },
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


static void def_watchers(lua_State *L) {
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

        /* additional watcher methods */
#define GET_WATCHER_MT(name)                            \
        lua_pushstring(L, TO_REG(name));                \
        lua_gettable(L, LUA_REGISTRYINDEX)
#define SETMETHOD(name, cfunc)                          \
        lua_pushcfunction(L, cfunc); lua_setfield(L, -2, #name)

        GET_WATCHER_MT(io);
        SETMETHOD(set, lev_io_set);
        SETMETHOD(fd, lev_io_get_fd);
        SETMETHOD(read, async_read);
        SETMETHOD(write, async_write);
        
        GET_WATCHER_MT(timer);
        SETMETHOD(set, lev_timer_set);
        
        GET_WATCHER_MT(periodic);
        SETMETHOD(set, lev_periodic_set);
        
        GET_WATCHER_MT(signal);
        SETMETHOD(set, lev_signal_set);
        SETMETHOD(signum, lev_signal_get_signum);
        
        GET_WATCHER_MT(child);
        SETMETHOD(set, lev_child_set);
        SETMETHOD(pid, lev_child_get_pid);
        
        GET_WATCHER_MT(stat);
        SETMETHOD(set, lev_stat_set);
        SETMETHOD(path, lev_stat_get_path);
        
        GET_WATCHER_MT(embed);
        SETMETHOD(set, lev_embed_set);
        SETMETHOD(sweep, lev_embed_sweep);

        GET_WATCHER_MT(async);
        SETMETHOD(send, lev_async_send);
        SETMETHOD(pending, lev_async_pending);
        lua_pop(L, 8);
}


int luaopen_evc(lua_State *L) {
        luaL_newmetatable(L, "Lev_loop");
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, loop_tostring);
        lua_setfield(L, -2, "__tostring");
        luaL_register(L, NULL, loop_mt);
        lua_pop(L, 1);

        def_watchers(L);

        lua_pushstring(L, err_handler_tag);
        lua_pushcfunction(L, def_error_handler);
        lua_settable(L, LUA_REGISTRYINDEX);

        luaL_register(L, "evc", evlib);
        return 1;
}
