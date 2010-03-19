#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <ev.h>

#include "evc.h"

/* TODO EV_MULTIPLICITY ? */

static int lev_version_major(lua_State *L) {
        lua_pushnumber(L, ev_version_major());
        return 1;
}

static int lev_version_minor(lua_State *L) {
        lua_pushnumber(L, ev_version_minor());
        return 1;
}
        

static int lev_supported_backends(lua_State *L) {
        lua_pushnumber(L, ev_supported_backends());
        return 1;
}


static int lev_recommended_backends(lua_State *L) {
        lua_pushnumber(L, ev_recommended_backends());
        return 1;
}


static int lev_embeddable_backends(lua_State *L) {
        lua_pushnumber(L, ev_embeddable_backends());
        return 1;
}


static int lev_time(lua_State *L) {
        ev_tstamp ts = ev_time();
        lua_pushnumber(L, ts);
        return 1;
}


static int lev_sleep(lua_State *L) {
        ev_tstamp ts = luaL_checknumber(L, 1);
        ev_sleep(ts);
        return 0;
}


/* ev_set_allocator (void *(*cb)(void *ptr, long size) -> void */
/* ev_set_syserr_cb (void *(*cb)(const char *msg) -> void */

static int lev_default_loop(lua_State *L) {
        int flags = luaL_optint(L, 1, 0);
        Lev_loop *lev_loop = (Lev_loop *)lua_newuserdata(L, sizeof(Lev_loop));
        struct ev_loop *loop = ev_default_loop(flags);
        lev_loop->t = loop;
        ev_set_userdata(loop, L);  /* so event loop knows the Lua state for callbacks*/
        luaL_getmetatable(L, "Lev_loop");
        lua_setmetatable(L, -2);
        return 1;               /* leaving the loop */
}


/* struct ev_loop *ev_loop_new (unsigned int flags); */
/* void ev_loop_destroy (EV_P); */
/* void ev_loop_fork (EV_P); */

/* ev_tstamp ev_now (EV_P); /\* time w.r.t. timers and the eventloop, updated after each poll *\/ */


static int lev_default_destroy(lua_State *L) {
        ev_default_destroy();
        return 0;
}


static int lev_default_fork(lua_State *L) {
        ev_default_fork();
        return 0;
}


static int lev_backend(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        lua_pushnumber(L, ev_backend(loop->t));
        return 1;
}


static int lev_now_update(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_now_update(loop->t);
        return 0;
}


static int lev_loop(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        int flags = luaL_optint(L, 2, 0);
        puts("Starting loop...\n");
        ev_loop(loop->t, flags);
        return 0;
}


static int lev_unloop(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        int flags = luaL_checknumber(L, 2);
        ev_unloop(loop->t, flags);
        return 0;
}


static int lev_ref(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_ref(loop->t);
        return 0;
}


static int lev_unref(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_unref(loop->t);
        return 0;
}


/* TODO ev_once */

static int lev_loop_count(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        lua_pushnumber(L, ev_loop_count(loop->t));
        return 1;
}


static int lev_loop_depth(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        lua_pushnumber(L, ev_loop_depth(loop->t));
        return 1;
}


static int lev_loop_verify(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_loop_verify(loop->t);
        return 0;
}


/* void ev_set_io_collect_interval (EV_P_ ev_tstamp interval); /\* sleep at least this time, default 0 *\/ */
/* void ev_set_timeout_collect_interval (EV_P_ ev_tstamp interval); /\* sleep at least this time, default 0 *\/ */

/* /\* advanced stuff for threading etc. support, see docs *\/ */
/* void ev_set_userdata (EV_P_ void *data); */
/* void *ev_userdata (EV_P); */
/* void ev_set_invoke_pending_cb (EV_P_ void (*invoke_pending_cb)(EV_P)); */
/* void ev_set_loop_release_cb (EV_P_ void (*release)(EV_P), void (*acquire)(EV_P)); */

static int lev_pending_count(lua_State *L) {
        //unsigned int ev_pending_count (EV_P); /* number of pending events, if any */
        return 0;
}


static int lev_invoke_pending(lua_State *L) {
        //void ev_invoke_pending (EV_P); /* invoke all pending watchers */
        return 0;
}


static int lev_suspend(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_suspend(loop->t);
        return 0;
}


static int lev_resume(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        ev_resume(loop->t);
        return 0;
}


/* #define ev_io_set(ev,fd_,events_)            do { (ev)->fd = (fd_); (ev)->events = (events_) | EV__IOFDSET; } while (0) */
/* #define ev_timer_set(ev,after_,repeat_)      do { ((ev_watcher_time *)(ev))->at = (after_); (ev)->repeat = (repeat_); } while (0) */
/* #define ev_periodic_set(ev,ofs_,ival_,rcb_)  do { (ev)->offset = (ofs_); (ev)->interval = (ival_); (ev)->reschedule_cb = (rcb_); } while (0) */
/* #define ev_signal_set(ev,signum_)            do { (ev)->signum = (signum_); } while (0) */
/* #define ev_child_set(ev,pid_,trace_)         do { (ev)->pid = (pid_); (ev)->flags = !!(trace_); } while (0) */
/* #define ev_stat_set(ev,path_,interval_)      do { (ev)->path = (path_); (ev)->interval = (interval_); (ev)->wd = -2; } while (0) */
/* #define ev_embed_set(ev,other_)              do { (ev)->other = (other_); } while (0) */
/* #define ev_async_set(ev)                     do { (ev)->sent = 0; } while (0) */

/* #define ev_io_init(ev,cb,fd,events)          do { ev_init ((ev), (cb)); ev_io_set ((ev),(fd),(events)); } while (0) */
/* #define ev_timer_init(ev,cb,after,repeat)    do { ev_init ((ev), (cb)); ev_timer_set ((ev),(after),(repeat)); } while (0) */
/* #define ev_periodic_init(ev,cb,ofs,ival,rcb) do { ev_init ((ev), (cb)); ev_periodic_set ((ev),(ofs),(ival),(rcb)); } while (0) */
/* #define ev_signal_init(ev,cb,signum)         do { ev_init ((ev), (cb)); ev_signal_set ((ev), (signum)); } while (0) */
/* #define ev_child_init(ev,cb,pid,trace)       do { ev_init ((ev), (cb)); ev_child_set ((ev),(pid),(trace)); } while (0) */
/* #define ev_stat_init(ev,cb,path,interval)    do { ev_init ((ev), (cb)); ev_stat_set ((ev),(path),(interval)); } while (0) */
/* #define ev_idle_init(ev,cb)                  do { ev_init ((ev), (cb)); ev_idle_set ((ev)); } while (0) */
/* #define ev_prepare_init(ev,cb)               do { ev_init ((ev), (cb)); ev_prepare_set ((ev)); } while (0) */
/* #define ev_check_init(ev,cb)                 do { ev_init ((ev), (cb)); ev_check_set ((ev)); } while (0) */
/* #define ev_embed_init(ev,cb,other)           do { ev_init ((ev), (cb)); ev_embed_set ((ev),(other)); } while (0) */
/* #define ev_fork_init(ev,cb)                  do { ev_init ((ev), (cb)); ev_fork_set ((ev)); } while (0) */
/* #define ev_async_init(ev,cb)                 do { ev_init ((ev), (cb)); ev_async_set ((ev)); } while (0) */

/* #define ev_is_pending(ev)                    (0 + ((ev_watcher *)(void *)(ev))->pending) /\* ro, true when watcher is waiting for callback invocation *\/ */
/* #define ev_is_active(ev)                     (0 + ((ev_watcher *)(void *)(ev))->active) /\* ro, true when the watcher has been started *\/ */


static int lev_priority(lua_State *L) {
        Lev_watcher *w = check_ev_watcher(L, 1);
        lua_pushnumber(L, ev_priority(w->t));
        return 1;
}

static int lev_set_priority(lua_State *L) {
        Lev_watcher *w = check_ev_watcher(L, 1);
        int prio = luaL_checknumber(L, 2);
        ev_set_priority((w->t), prio);
        return 0;
}


/* static int lev_set_cb(lua_State *L) { */
/*         Lev_watcher *w = check_ev_watcher(L, 1); */
/*         Lev_callback *c = check_ev_callback(L, 2); */
/*         ev_set_cb(w->t, c->t); */
/*         return 0; */
/* } */


/* feed various events */


/*************/
/* Callbacks */
/*************/

static void call_luafun_cb(struct ev_loop *l, ev_watcher *w, int events) {
        lua_State *L = ev_userdata(l);
        lua_pushlightuserdata(L, w);
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (lua_isthread(L, -1)) {
                puts("About to resume coroutine\n");
                lua_State *Coro = (void *)lua_topointer(L, -1);
                if (Coro == NULL) printf("CRAP\n");
                lua_pushlightuserdata(Coro, w);
                lua_pushinteger(Coro, events);
                lua_resume(Coro, 2);
        } else if (lua_isfunction(L, -1)) {
                puts("About to call Lua callback fun\n");
                lua_pushlightuserdata(L, w);
                lua_pushinteger(L, events);
                lua_pcall(L, 2, 0, 0);
        } else {
                lua_pushstring(L, "Not a coroutine or function");
                lua_error(L);
        }
}


/* Set a callback: [ ev_watcher *w, luafun or coro ] */
static int lev_set_cb(lua_State *L) {
        /* settable: t k v -> t[k] = v */
        Lev_watcher *watcher = check_ev_watcher(L, 1);
        ev_watcher *w = watcher->t;
        if (lua_isthread(L, 2)) {          /* coro goes in watcher */
                puts("Setting coro callback\n");
/*                 w->data = (void *)lua_topointer(L, 2); */
        } else if (lua_isfunction(L, 2)) { /* reg[udata] = fun */
                puts("Setting fun callback\n");
        }
        lua_pushlightuserdata(L, w);
        lua_insert(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
        return 0;
}


/*********************/
/* Watcher functions */
/*********************/

/* static int lev_io_init(lua_State *L) { */
/*         Lev_watcher *lw = (Lev_watcher *)lua_newuserdata(L, sizeof(Lev_watcher)); */
/*         ev_io *w = (ev_io *)lw->t; */
/*         Lev_callback *cb = check_ev_callback(L, 1); */
/*         int fdes = luaL_checknumber(L, 2); */
/*         int event_mask = luaL_checknumber(L, 3); */
/*         ev_io_init(w, (void *) cb->t, fdes, event_mask); */
/*         return 1; */
/* } */


/* static int lev_io_start(lua_State *L) { */
/*         Lev_loop *l = check_ev_loop(L, 1); */
/*         Lev_watcher *w = check_ev_watcher(L, 2); */
/*         ev_io_start(l->t, (ev_io *) w->t); */
/*         return 0; */
/* } */


static int lev_timer_init(lua_State *L) {
        double after = luaL_checknumber(L, 2);
        double repeat = luaL_optint(L, 3, 0);
        lua_pop(L, 3);
        Lev_watcher *watcher = (Lev_watcher *)lua_newuserdata(L, sizeof(Lev_watcher *));
        luaL_getmetatable(L, "Lev_watcher");
        lua_setmetatable(L, -2);
        ev_timer *w = (ev_timer *)malloc(sizeof(ev_timer));
        watcher->t = (ev_watcher *)w;
        puts("TI");
        ev_timer_init(w, (void *)call_luafun_cb, after, repeat);
        w->data = NULL;
        return 1;
}


static int lev_timer_start(lua_State *L) {
        Lev_loop *loop = check_ev_loop(L, 1);
        Lev_watcher *watcher = check_ev_watcher(L, 2);

        ev_timer *t = (ev_timer *) watcher->t; /* check watcher type */
        ev_timer_start(loop->t, t);
        return 0;
}


/**********/
/* Module */
/**********/

static const struct luaL_Reg loop_mt [] = {
        { "loop", lev_loop },
        { "timer_init", lev_timer_init },
        { "timer_start", lev_timer_start },
        { NULL, NULL },
};


static const struct luaL_Reg watcher_mt [] = {
        { "set_cb", lev_set_cb },
/*         { "timer_init", lev_timer_init }, */
/*         { "timer_start", lev_timer_start }, */
        { NULL, NULL },
};


static const struct luaL_Reg evlib[] = {
        { "version_major", lev_version_major },
        { "version_minor", lev_version_minor },
/*         { "set_cb", lev_set_cb }, */
        { "set_priority", lev_set_priority },
        { "priority", lev_priority },
/*         { "timer_init", lev_timer_init }, */
        { "resume", lev_resume },
        { "suspend", lev_suspend },
        { "invoke_pending", lev_invoke_pending },
        { "pending_count", lev_pending_count },
        { "loop_verify", lev_loop_verify },
        { "loop_depth", lev_loop_depth },
        { "loop_count", lev_loop_count },
        { "unref", lev_unref },
        { "ref", lev_ref },
        { "unloop", lev_unloop },
        { "loop", lev_loop },
        { "now_update", lev_now_update },
        { "backend", lev_backend },
        { "default_fork", lev_default_fork },
        { "default_destroy", lev_default_destroy },
        { "default_loop", lev_default_loop },
        { "sleep", lev_sleep },
        { "time", lev_time },
        { "embeddable_backends", lev_embeddable_backends },
        { "recommended_backends", lev_recommended_backends },
        { "supported_backends", lev_supported_backends },
/*         { "io_init", lev_io_init }, */
/*         { "io_start", lev_io_start }, */
/*         { "timer_init", lev_timer_init }, */
/*         { "timer_start", lev_timer_start }, */
        { "set_cb", lev_set_cb },
        { NULL, NULL },
};


/* static void init_callbacks(lua_State *L) { */
/*         Lev_callback *cf = (Lev_callback *)lua_newuserdata(L, sizeof(Lev_callback)); */
/*         cf->t = call_luafun_cb; */
/*         lua_pushlightuserdata(L, cf); */
/*         lua_setfield(L, -2, "luafun_cb"); */
/* } */


int luaopen_evc(lua_State *L) {
        luaL_newmetatable(L, "Lev_loop");
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_register(L, NULL, loop_mt);

        luaL_newmetatable(L, "Lev_watcher");
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_register(L, NULL, watcher_mt);

/*         init_callbacks(L); */

        luaL_register(L, "evc", evlib);
        return 1;
}
