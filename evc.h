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

#define check_ev_loop(L, n) ((Lev_loop *)luaL_checkudata(L, n, "Lev_loop"))
#define check_ev_watcher(L, n) ((Lev_watcher *)luaL_checkudata(L, n, "Lev_watcher"))
#define check_ev_callback(L, n) ((Lev_callback *)luaL_checkudata(L, n, "Lev_callback"))
#define check_ev_callback(L, n) ((Lev_callback *)luaL_checkudata(L, n, "Lev_callback"))

#endif

