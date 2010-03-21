#ifndef LEVC_H
#define LEVC_H

#define DEBUG 0
#define BUFSZ 1024

/**************/
/* Prototypes */
/**************/

static void call_luafun_cb(struct ev_loop *l, ev_watcher *w, int events);


/**********/
/* Macros */
/**********/

#define PRE_L(name)  l##name
#define MTNAME(name) name##_mt
#define TOS(n) #n
#define PRE_LEV(name) Lev_##name
#define PRE_EV(name) ev_##name
#define TO_REG(name) TOS(Lev_##name)

#define pushnum(fun)                                \
        static int PRE_L(fun)(lua_State *L) {       \
        lua_pushnumber(L, fun());                   \
        return 1;                                   \
}

#define push_backend_table(fun)                     \
        static int PRE_L(fun)(lua_State *L) {       \
        backends_to_table(L, fun());                \
        return 1;                                   \
}

#define loop_fun_0(fun)                             \
        static int PRE_L(fun)(lua_State *L) {       \
        Lev_loop *loop = check_ev_loop(L, 1);       \
        fun(loop->t);                               \
        return 0;                                   \
}

#define loop_fun_n1(fun)                            \
        static int PRE_L(fun)(lua_State *L) {       \
        Lev_loop *loop = check_ev_loop(L, 1);       \
        lua_pushnumber(L, fun(loop->t));            \
        return 1;                                   \
}

#define watcher_bool(fun)                           \
        static int PRE_L(fun)(lua_State *L) {       \
        Lev_watcher *w = check_watcher(L, 1);       \
        lua_pushboolean(L, fun(w->t));              \
        return 1;                                   \
}

#define DEFMETATABLE(name)                          \
        luaL_newmetatable(L, TO_REG(name));         \
        lua_pushvalue(L, -1);                       \
        lua_setfield(L, -2, "__index");             \
        luaL_register(L, NULL, MTNAME(name))


#define DEF_WATCHER_METATABLE(name)                 \
        luaL_newmetatable(L, TO_REG(name));         \
        lua_pushboolean(L, 1);                      \
        lua_setfield(L, -2, "__watcher");           \
        lua_pushcfunction(L, name##_tostring);      \
        lua_setfield(L, -2, "__tostring");          \
        lua_pushvalue(L, -1);                       \
        lua_setfield(L, -1, "__index");             \
        luaL_register(L, NULL, MTNAME(name));       \
        lua_pop(L, 1)



#define DEFWATCHER(type)                            \
        typedef struct PRE_LEV(type) {              \
        ev_##type *t;                               \
        } PRE_LEV(type)


#define check_ev_loop(L, n) ((Lev_loop *)luaL_checkudata(L, n, "Lev_loop"))
#define check_ev_callback(L, n) ((Lev_callback *)luaL_checkudata(L, n, "Lev_callback"))

#define CHECK_WATCHER(n, type)                      \
        (Lev_##type *)luaL_checkudata(L, n, TO_REG(type))

#define TOSTRING_WATCHER(type)                      \

#define DEF_WATCHER_METHODS(type)                   \
        static int lev_##type##_start(lua_State *L){  \
        PRE_LEV(type) *w = CHECK_WATCHER(1, type);  \
        Lev_loop *loop = check_ev_loop(L, 2);       \
        ev_##type##_start(loop->t, w->t);           \
        return 0;                                   \
        }                                           \
        static int lev_##type##_stop(lua_State *L){ \
        PRE_LEV(type) *w = CHECK_WATCHER(1, type);  \
        Lev_loop *loop = check_ev_loop(L, 2);       \
        ev_##type##_stop(loop->t, w->t);            \
        return 0;                                   \
        }                                           \
        static int type##_tostring(lua_State *L) {  \
        PRE_LEV(type) *w = CHECK_WATCHER(1, type);  \
        lua_pushfstring(L, "%s_watcher: %p", #type, w); \
        return 1;                                   \
        }

/*  PRE_LEV(name) *name = (PRE_LEV(name) *) lua_newuserdata(L, sizeof(PRE_LEV(name)) *); \ */


#define ALLOC_UDATA_AND_WATCHER(name)               \
        PRE_LEV(name) *name = (PRE_LEV(name) *)lua_newuserdata(L, sizeof(PRE_LEV(name)*)); \
        luaL_getmetatable(L, TO_REG(name));         \
        lua_setmetatable(L, -2);                    \
        PRE_EV(name) *t = (PRE_EV(name) *)malloc(sizeof(PRE_EV(name))); \
        name->t = (PRE_EV(name) *) t;

#define REGISTER_WATCHER(name)                  \
        lua_pushlightuserdata(L, name->t);      \
        lua_pushvalue(L, -2);                   \
        lua_settable(L, LUA_REGISTRYINDEX)

#define DEF_WATCHER_MT_VALS(name)                   \
        static const struct luaL_Reg name##_mt [] = { \
        { "start", lev_##name##_start },            \
        { "stop", lev_##name##_stop },              \
        { "set_cb", lev_set_cb },                   \
        { "is_active", lev_is_active },             \
        { "is_pending", lev_is_pending },           \
        { "clear_pending", lev_clear_pending },     \
        { "set_priority", lev_set_priority },       \
        { "priority", lev_priority },               \
        { NULL, NULL },                             \
};


#define setbitfield(name) \
        lua_pushboolean(L, 1); lua_setfield(L, -2, name);


/***********/
/* Structs */
/***********/

/* Full userdata. Mostly used as a key to look up Lua-side callbacks. */

typedef struct Lev_loop {
        struct ev_loop *t;
} Lev_loop;

DEFWATCHER(watcher);            /* generic */
DEFWATCHER(io);
DEFWATCHER(timer);
DEFWATCHER(periodic);
DEFWATCHER(signal);
DEFWATCHER(child);
DEFWATCHER(stat);
DEFWATCHER(idle);
DEFWATCHER(prepare);
DEFWATCHER(check);
DEFWATCHER(embed);
DEFWATCHER(fork);
DEFWATCHER(async);

#endif
