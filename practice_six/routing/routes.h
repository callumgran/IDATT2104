#ifndef ROUTES_H
#define ROUTES_H

#include "binarytree.h"

/* Structs */

struct routes_t {
    struct tree_t *base;
};

struct route_t {
    char *key;
    char *value;
};

/* Methods */

/*
    Function used to initialize a routes.
    @param routes struct routes_t*: routes to initialize.
*/
void init_routes(struct routes_t *routes);

/*
    Function used to create a new route.
    @param key char*: key of the route.
    @param value char*: value of the route.
    @return struct route_t*: new route.
*/
struct route_t *create_route(char *key, char *value);

/*
    Function used to insert a new route in a routes.
    @param routes struct routes_t*: routes to insert the route.
    @param route struct route_t*: route to insert.
*/
void insert_route(struct routes_t *routes, struct route_t *route);

/*
    Function used to remove a route from a routes.
    @param routes struct routes_t*: routes to remove the route.
    @param key char*: key of the route to remove.
*/
void remove_route(struct routes_t *routes, char *key);

/*
    Function used to find a route in a routes.
    @param routes struct routes_t*: routes to find the route.
    @param key char*: key of the route to find.
    @return struct route_t*: found route.
*/
struct route_t *find_route(struct routes_t *routes, char *key);

/*
    Function used to free a route.
    @param route struct route_t*: route to free.
*/
void free_route(void *route);

/*
    Function used to free a routes.
    @param routes struct routes_t*: routes to free.
*/
void free_routes(struct routes_t *routes);

#endif