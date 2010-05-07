#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include <ev.h>

/* #include "evc.h" */


static void pr_stack(lua_State *L) {
        int ct = lua_gettop(L);
        printf("Stack height is %d\n", ct);
        int i;
        for (i=1; i <= ct; i++)
                printf("%d: %d %s %lx\n",
                    i, lua_type(L, i),
                    lua_tostring(L, i), (long)lua_topointer(L, i));
}


static void do_error(lua_State *L, const char *e) {
        lua_pushstring(L, e); lua_error(L);
}


static void call_luafun_cb(lua_State *L) {
        int res = 0;
        lua_pushstring(L, "bacon");
        lua_gettable(L, LUA_REGISTRYINDEX);

        if (lua_isthread(L, -1)) {
                lua_State *Coro = (void *)lua_topointer(L, -1);
                lua_pushstring(Coro, "Bananas");
                puts("About to resume...");
                res = lua_resume(Coro, 1);
                printf("After resume, %d\n", res);
                if (res > LUA_YIELD) {
                        puts("*** coro crashed");
                        pr_stack(Coro);
                        pr_stack(L);

                        lua_pushstring(L, "bacon");
                        lua_pushnil(L);
                        lua_settable(L, LUA_REGISTRYINDEX);
                }
        } else if (lua_isfunction(L, -1)) {
                lua_pushstring(L, "Bananas");
                puts("About to call fun");
                res = lua_pcall(L, 1, 0, 0);
                printf("After callback fun, %d\n", res);
                if (res || lua_gettop(L) > 1) {
                        printf("Error!\n");
                        pr_stack(L);
                }
        } else {
                puts("nipe");
                lua_pop(L, 1);
        }
}


static int setcb(lua_State *L) {
        puts("in set");
        pr_stack(L);
        if (lua_isthread(L, -1) || lua_isfunction(L, -1)) {
                lua_pushstring(L, "bacon");
                lua_insert(L, -2);
                lua_settable(L, LUA_REGISTRYINDEX);
                puts("set");
        } else {
                puts("not fun or thread");
        }
        return 0;
}


static int runcb(lua_State *L) {
        puts("run");
        call_luafun_cb(L);
        return 0;
}


static const struct luaL_Reg crashylib[] = {
        { "set", setcb },
        { "run", runcb },
        { NULL, NULL }
};


int luaopen_crashy(lua_State *L) {
        luaL_register(L, "crashy", crashylib);
        return 1;
}
