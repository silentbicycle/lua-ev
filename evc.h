#ifndef LEVC_H
#define LEVC_H

typedef struct Lev_loop {
        struct ev_loop *t;
} Lev_loop;

/* Wrapped userdata, for GC. */

/* Light userdata. Mostly used as a key to look up Lua-side callbacks. */
typedef struct Lev_watcher {
        ev_watcher *t;
} Lev_watcher;

#ifdef EV_MULTIPLICITY
typedef struct Lev_callback {
        void (*t)(struct ev_loop *loop, ev_watcher *w, int events);
} Lev_callback;
#endif


/**************/
/* Prototypes */
/**************/

static void call_luafun_cb(struct ev_loop *l, ev_watcher *w, int events);


/**********/
/* Macros */
/**********/

#define TOLEV(name)  l##name
#define MTNAME(name) name##_mt
#define TOS(n) #n
#define TOREG(name) TOS(Lev_##name)

#define pushnum(fun)                                \
        static int TOLEV(fun)(lua_State *L) {       \
        lua_pushnumber(L, fun());                   \
        return 1;                                   \
}

#define loop_fun_0(fun)                             \
        static int TOLEV(fun)(lua_State *L) {       \
        Lev_loop *loop = check_ev_loop(L, 1);       \
        fun(loop->t);                               \
        return 0;                                   \
}

#define loop_fun_n1(fun)                            \
        static int TOLEV(fun)(lua_State *L) {       \
        Lev_loop *loop = check_ev_loop(L, 1);       \
        lua_pushnumber(L, fun(loop->t));            \
        return 1;                                   \
}

#define watcher_bool(fun)                           \
        static int TOLEV(fun)(lua_State *L) {       \
        Lev_watcher *w = check_ev_watcher(L, 1);    \
        lua_pushboolean(L, fun(w->t));              \
        return 1;                                   \
}

#define defmetatable(name)                          \
        luaL_newmetatable(L, TOREG(name));          \
        lua_pushvalue(L, -1);                       \
        lua_setfield(L, -2, "__index");             \
        luaL_register(L, NULL, MTNAME(name))


#define check_ev_loop(L, n) ((Lev_loop *)luaL_checkudata(L, n, "Lev_loop"))
#define check_ev_watcher(L, n) ((Lev_watcher *)luaL_checkudata(L, n, "Lev_watcher"))
#define check_ev_callback(L, n) ((Lev_callback *)luaL_checkudata(L, n, "Lev_callback"))
#define check_ev_callback(L, n) ((Lev_callback *)luaL_checkudata(L, n, "Lev_callback"))

#endif
