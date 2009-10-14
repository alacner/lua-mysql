#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#include <winsock2.h>
#define NO_CLIENT_LONG_LONG
#endif

#include "mysql.h"

#include "lua.h"
#include "lauxlib.h"
#if ! defined (LUA_VERSION_NUM) || LUA_VERSION_NUM < 501
#include "compat-5.1.h"
#endif

/* For compat with old version 4.0 */
#if (MYSQL_VERSION_ID < 40100) 
#define MYSQL_TYPE_VAR_STRING   FIELD_TYPE_VAR_STRING 
#define MYSQL_TYPE_STRING       FIELD_TYPE_STRING 
#define MYSQL_TYPE_DECIMAL      FIELD_TYPE_DECIMAL 
#define MYSQL_TYPE_SHORT        FIELD_TYPE_SHORT 
#define MYSQL_TYPE_LONG         FIELD_TYPE_LONG 
#define MYSQL_TYPE_FLOAT        FIELD_TYPE_FLOAT 
#define MYSQL_TYPE_DOUBLE       FIELD_TYPE_DOUBLE 
#define MYSQL_TYPE_LONGLONG     FIELD_TYPE_LONGLONG 
#define MYSQL_TYPE_INT24        FIELD_TYPE_INT24 
#define MYSQL_TYPE_YEAR         FIELD_TYPE_YEAR 
#define MYSQL_TYPE_TINY         FIELD_TYPE_TINY 
#define MYSQL_TYPE_TINY_BLOB    FIELD_TYPE_TINY_BLOB 
#define MYSQL_TYPE_MEDIUM_BLOB  FIELD_TYPE_MEDIUM_BLOB 
#define MYSQL_TYPE_LONG_BLOB    FIELD_TYPE_LONG_BLOB 
#define MYSQL_TYPE_BLOB         FIELD_TYPE_BLOB 
#define MYSQL_TYPE_DATE         FIELD_TYPE_DATE 
#define MYSQL_TYPE_NEWDATE      FIELD_TYPE_NEWDATE 
#define MYSQL_TYPE_DATETIME     FIELD_TYPE_DATETIME 
#define MYSQL_TYPE_TIME         FIELD_TYPE_TIME 
#define MYSQL_TYPE_TIMESTAMP    FIELD_TYPE_TIMESTAMP 
#define MYSQL_TYPE_ENUM         FIELD_TYPE_ENUM 
#define MYSQL_TYPE_SET          FIELD_TYPE_SET
#define MYSQL_TYPE_NULL         FIELD_TYPE_NULL

#define mysql_commit(_) ((void)_)
#define mysql_rollback(_) ((void)_)
#define mysql_autocommit(_,__) ((void)_)

#endif

/* Compatibility between Lua 5.1+ and Lua 5.0 */
#ifndef LUA_VERSION_NUM
#define LUA_VERSION_NUM 0
#endif
#if LUA_VERSION_NUM < 501
#define luaL_register(a, b, c) luaL_openlib((a), (b), (c), 0)
#endif

#define LUA_MYSQL_CONN "MySQL connection"
#define LUA_MYSQL_RES "MySQL resource"
#define LUA_MYSQL_TABLENAME "mysql"

typedef struct {
    short      closed;
} pseudo_data;

typedef struct {
    short   closed;
    int     env;
    MYSQL   *conn;
} lua_mysql_conn;

typedef struct {
    short      closed;
    int        conn;               /* reference to connection */
    int        numcols;            /* number of columns */
    int        colnames, coltypes; /* reference to column information tables */
    MYSQL_RES *res;
} lua_mysql_res;

/**
** common functions Part
*/

/*
** Get the internal database type of the given column.
*/
static char *luaM_getcolumntype (enum enum_field_types type) {

    switch (type) {
        case MYSQL_TYPE_VAR_STRING: case MYSQL_TYPE_STRING:
            return "string";
        case MYSQL_TYPE_DECIMAL: case MYSQL_TYPE_SHORT: case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT: case MYSQL_TYPE_DOUBLE: case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_INT24: case MYSQL_TYPE_YEAR: case MYSQL_TYPE_TINY:
            return "number";
        case MYSQL_TYPE_TINY_BLOB: case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB: case MYSQL_TYPE_BLOB:
            return "binary";
        case MYSQL_TYPE_DATE: case MYSQL_TYPE_NEWDATE:
            return "date";
        case MYSQL_TYPE_DATETIME:
            return "datetime";
        case MYSQL_TYPE_TIME:
            return "time";
        case MYSQL_TYPE_TIMESTAMP:
            return "timestamp";
        case MYSQL_TYPE_ENUM: case MYSQL_TYPE_SET:
            return "set";
        case MYSQL_TYPE_NULL:
            return "null";
        default:
            return "undefined";
    }
}

/*                   
** Return the name of the object's metatable.
** This function is used by `tostring'.     
*/                            
static int luaM_tostring (lua_State *L) {                
    char buff[100];             
    pseudo_data *obj = (pseudo_data *)lua_touserdata (L, 1);     
    if (obj->closed)                          
        strcpy (buff, "closed");
    else
        sprintf (buff, "%p", (void *)obj);
    lua_pushfstring (L, "%s (%s)", lua_tostring(L,lua_upvalueindex(1)), buff);
    return 1;                            
}       

/*
** Define the metatable for the object on top of the stack
*/
void luaM_setmeta (lua_State *L, const char *name) {
    luaL_getmetatable (L, name);
    lua_setmetatable (L, -2);
}     

/*
** Create a metatable and leave it on top of the stack.
*/
int luaM_register (lua_State *L, const char *name, const luaL_reg *methods) {
    if (!luaL_newmetatable (L, name))
        return 0;

    /* define methods */
    luaL_register (L, NULL, methods);

    /* define metamethods */
    lua_pushliteral (L, "__gc");
    lua_pushcfunction (L, methods->func);
    lua_settable (L, -3);

    lua_pushliteral (L, "__index");
    lua_pushvalue (L, -2);
    lua_settable (L, -3);

    lua_pushliteral (L, "__tostring");
    lua_pushstring (L, name);
    lua_pushcclosure (L, luaM_tostring, 1);
    lua_settable (L, -3);

    lua_pushliteral (L, "__metatable");
    lua_pushliteral (L, "you're not allowed to get this metatable");
    lua_settable (L, -3);

    return 1;
}

/*
** message
*/

static int luaM_msg(lua_State *L, const int n, const char *m) {
    if (n) {
        lua_pushnumber(L, 1);
    } else {
        lua_pushnil(L);
    }
    lua_pushstring(L, m);
    return 2;
}

/*
** Push the value of #i field of #tuple row.
*/
static void pushvalue (lua_State *L, void *row, long int len) {
    if (row == NULL)
        lua_pushnil (L);
    else
        lua_pushlstring (L, row, len);
}


/**
** Handle Part
*/

/*
** Check for valid connection.
*/
static lua_mysql_conn *Mget_conn (lua_State *L) {
    lua_mysql_conn *my_conn = (lua_mysql_conn *)luaL_checkudata (L, 1, LUA_MYSQL_CONN);
    luaL_argcheck (L, my_conn != NULL, 1, "connection expected");
    luaL_argcheck (L, !my_conn->closed, 1, "connection is closed");
    return my_conn;
}

/*
** Check for valid resource.
*/
/*
static cur_data *Mget_res (lua_State *L) {
    lua_mysql_res *my_res = (lua_mysql_res *)luaL_checkudata (L, 1, LUA_MYSQL_RES);
    luaL_argcheck (L, my_res != NULL, 1, "resource expected");
    luaL_argcheck (L, !my_res->closed, 1, "resource is closed");
    return my_res;
}
*/

static int Lmysql_connect (lua_State *L) {
    lua_mysql_conn *my_conn = (lua_mysql_conn *)lua_newuserdata(L, sizeof(lua_mysql_conn));
    luaM_setmeta (L, LUA_MYSQL_CONN);

    char *host = NULL, *delim = ":";
    const char *host_and_port = luaL_optstring(L, 1, NULL);
    const char *user = luaL_optstring(L, 2, NULL);
    const char *passwd = luaL_optstring(L, 3, NULL);

    int port = MYSQL_PORT;
    
    MYSQL *conn;

    conn = mysql_init(NULL);  
    if ( ! conn) {  
        return luaM_msg (L, 0, "Error: mysql_init failed !");
    }

    host = strtok(host_and_port, delim);
    port = strtok(NULL, delim);

    if ( ! mysql_real_connect(conn, host, user, passwd, NULL, port, NULL, 0)) {
        char error_msg[100];
        strncpy (error_msg,  mysql_error(conn), 99);
        mysql_close (conn); /* Close conn if connect failed */
        return luaM_msg (L, 0, error_msg);
    }

    /* fill in structure */
    my_conn->closed = 0;
    my_conn->env = LUA_NOREF;
    my_conn->conn = conn;
    //lua_pushvalue (L, env);
    //my_conn->env = luaL_ref (L, LUA_REGISTRYINDEX);
    return 1;
}

/**
 Close MySQL connection
*/
static int Lmysql_close (lua_State *L) {
    lua_mysql_conn *my_conn = Mget_conn (L);
    luaL_argcheck (L, my_conn != NULL, 1, "connection expected");
    if (my_conn->closed) {
        lua_pushboolean (L, 0);
        return 1;
    }

    my_conn->closed = 1;
    luaL_unref (L, LUA_REGISTRYINDEX, my_conn->env);
    mysql_close (my_conn->conn);
    lua_pushboolean (L, 1);
    return 1;
}


/*
** Creates the metatables for the objects and registers the
** driver open method.
*/
int luaopen_mysql (lua_State *L) {
    struct luaL_reg driver[] = {
        { "connect",   Lmysql_connect },
        { NULL, NULL },
    };

    struct luaL_reg resource_methods[] = {
        { NULL, NULL }
    };

    static const luaL_reg connection_methods[] = {
        { "connect",   Lmysql_connect },
        { "close",   Lmysql_close },
        { NULL, NULL }
    };

    luaM_register (L, LUA_MYSQL_CONN, connection_methods);
    luaM_register (L, LUA_MYSQL_RES, resource_methods);
    lua_pop (L, 2);

    luaL_register (L, LUA_MYSQL_TABLENAME, driver);

    lua_pushliteral (L, "_MYSQLVERSION");
    lua_pushliteral (L, MYSQL_SERVER_VERSION);   
    lua_settable (L, -3);     

    return 1;
}

