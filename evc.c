#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <ev.h>

#include "evc.h"

/* TODO EV_MULTIPLICITY ? */


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

static int init_loop_mt(lua_State *L, Lev_loop *lev_loop, struct ev_loop *loop) {
        lev_loop->t = loop;
        ev_set_userdata(loop, L);  /* so event loop knows the Lua state for callbacks*/
        luaL_getmetatable(L, "Lev_loop");
        lua_setmetatable(L, -2);
        return 1;               /* leaving the loop */
}

static int lev_default_loop(lua_State *L) {
        int flags = luaL_optint(L, 1, 0);
        Lev_loop *lev_loop = (Lev_loop *)lua_newuserdata(L, sizeof(Lev_loop));
        struct ev_loop *loop = ev_default_loop(flags);
        return init_loop_mt(L, lev_loop, loop);
}

static int lev_loop_new(lua_State *L) {
        int flags = luaL_optint(L, 1, 0);
        Lev_loop *lev_loop = (Lev_loop *)lua_newuserdata(L, sizeof(Lev_loop));
        struct ev_loop *loop = ev_loop_new(flags);
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


/*********************/
/* Watcher functions */
/*********************/

watcher_bool(ev_is_active)
watcher_bool(ev_is_pending)


/* Set a callback: [ ev_watcher *w, luafun or coro ] */
static int lev_set_cb(lua_State *L) {
        /* settable: t k v -> t[k] = v */
        Lev_watcher *watcher = check_ev_watcher(L, 1);
        ev_watcher *w = watcher->t;
        if (lua_isthread(L, 2)) {          /* coro goes in watcher */
                puts("Setting coro callback\n");
        } else if (lua_isfunction(L, 2)) { /* reg[udata] = fun */
                puts("Setting fun callback\n");
        }
        lua_pushlightuserdata(L, w);
        lua_insert(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
        return 0;
}

static int lev_set_priority(lua_State *L) {
        Lev_watcher *w = check_ev_watcher(L, 1);
        int prio = luaL_checknumber(L, 2);
        ev_set_priority((w->t), prio);
        return 0;
}

static int lev_priority(lua_State *L) {
        Lev_watcher *w = check_ev_watcher(L, 1);
        lua_pushnumber(L, ev_priority(w->t));
        return 1;
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
        { NULL, NULL },
};


static const struct luaL_Reg loop_mt [] = {
        { "loop", lev_loop },
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
        { "unloop", lev_unloop },
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
        { "timer_init", lev_timer_init },
        { "timer_start", lev_timer_start },
        { NULL, NULL },
};


static const struct luaL_Reg watcher_mt [] = {
        { "set_cb", lev_set_cb },
        { "is_active", lev_is_active },
        { "is_pending", lev_is_pending },
        { "set_priority", lev_set_priority },
        { "priority", lev_priority },
/*         { "timer_init", lev_timer_init }, */
/*         { "timer_start", lev_timer_start }, */
        { NULL, NULL },
};


int luaopen_evc(lua_State *L) {
        defmetatable(loop);
        defmetatable(watcher);

        luaL_register(L, "evc", evlib);
        return 1;
}
