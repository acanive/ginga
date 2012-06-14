/* nclua-luax.c -- Auxiliary Lua functions.
   Copyright (C) 2006-2012 PUC-Rio/Laboratorio TeleMidia

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc., 51
Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <assert.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>

#include "nclua.h"
#include "nclua-util-private.h"
#include "nclua-luax-private.h"

/* Sets t[K] to nil, where t is the table at index INDEX.  */

void
ncluax_unsetfield (lua_State *L, int index, const char *k)
{
  lua_pushnil (L);
  lua_setfield (L, ncluax_abs (L, index), k);
}

#define NCLUAX_GETXFIELD_BODY(lua_isx, lua_tox) \
  {                                             \
    nclua_bool_t status = FALSE;                \
    lua_getfield (L, ncluax_abs (L, index), k); \
    if (likely (lua_isx (L, -1)))               \
      {                                         \
        status = TRUE;                          \
        *v = lua_tox (L, -1);                   \
      }                                         \
    lua_pop (L, 1);                             \
    return status;                              \
  }

/* If t[K] is an integer, stores it in *V and returns true.
   Otherwise, returns false.  */

nclua_status_t
ncluax_getintfield (lua_State *L, int index, const char *k, int *v)
{
  NCLUAX_GETXFIELD_BODY (lua_isnumber, lua_tointeger)
}

/* If t[K] is a number, stores it in *V and returns true.
   Otherwise, returns false.  */

nclua_status_t
ncluax_getnumberfield (lua_State *L, int index, const char *k, double *v)
{
  NCLUAX_GETXFIELD_BODY (lua_isnumber, lua_tonumber)
}

/* If t[K] is a string, stores it in *V and returns true.
   Otherwise, returns false.  */

nclua_status_t
ncluax_getstringfield (lua_State *L, int index, const char *k,
                       const char **v)
{
  NCLUAX_GETXFIELD_BODY (lua_isstring, lua_tostring)
}

static void
ncluax_tableinsert_tail (lua_State *L, int index, int pos,
                         void (*lua_gettable_fn) (lua_State *, int),
                         void (*lua_settable_fn) (lua_State *, int))
{
  int t = ncluax_abs (L, index);
  int n = lua_objlen (L, t);
  pos = min (max (pos, 1), n + 1);

  /* Shift up other elements, before inserting.  */
  if (pos <= n)
    {
      int i;
      for (i = n; i >= pos; i--)
        {
          lua_pushinteger (L, i);
          lua_gettable_fn (L, t);

          lua_pushinteger (L, i + 1);
          lua_insert (L, -2);
          lua_settable_fn (L, t);
        }
    }

  /* Insert the new element into table.  */
  lua_pushinteger (L, pos);
  lua_pushvalue (L, -2);
  lua_settable_fn (L, t);
}

/* Inserts the element at top of stack in position POS of table at index
   INDEX.  Other elements are shifted up to open space, if necessary.  */

void
ncluax_tableinsert (lua_State *L, int index, int pos)
{
  ncluax_tableinsert_tail (L, index, pos, lua_gettable, lua_settable);
}

/* Similar to ncluax_tableinsert(), but does a raw access;
   i.e., without metamethods.  */

void
ncluax_rawinsert (lua_State *L, int index, int pos)
{
  ncluax_tableinsert_tail (L, index, pos, lua_rawget, lua_rawset);
}

static void
ncluax_tableremove_tail (lua_State *L, int index, int pos,
                         void (*lua_gettable_fn) (lua_State *, int),
                         void (*lua_settable_fn) (lua_State *, int))
{
  int i;
  int t = ncluax_abs (L, index);
  int n = lua_objlen (L, t);

  if (unlikely (pos < 1 || pos > n))
    return;                     /* nothing to do */

  for (i = pos; i <= n; i++)
    {
      lua_pushinteger (L, i + 1);
      lua_gettable_fn (L, t);

      lua_pushinteger (L, i);
      lua_insert (L, -2);
      lua_settable_fn (L, t);
    }
}

/* Removes the element in position POS of table at index INDEX.
   Other elements are shifted down to close the space, if necessary.  */

void
ncluax_tableremove (lua_State *L, int index, int pos)
{
  ncluax_tableremove_tail (L, index, pos, lua_gettable, lua_settable);
}

/* Similar to ncluax_tableremove(), but does a raw access;
   i.e., without metamethods.  */

void
ncluax_rawremove (lua_State *L, int index, int pos)
{
  ncluax_tableremove_tail (L, index, pos, lua_rawget, lua_rawset);
}

static void
ncluax_print_error (lua_State *L, int level, const char *prefix,
                    const char *format, va_list args)
{
  nclua_bool_t space = FALSE;
  const char *where;

  fflush (stdout);

  if (prefix != NULL)
    {
      fputs (prefix, stderr);
      space = TRUE;
    }

  luaL_where (L, level);
  where = lua_tostring (L, -1);
  lua_pop (L, 1);
  if (where != NULL)
    {
      fputs (where, stderr);
      space = TRUE;
    }

  vfprintf (stderr, format, args);
  fputc ('\n', stderr);
  fflush (stderr);
}

/* Outputs Lua error message at level LEVEL to standard error.  */

void
ncluax_error (lua_State *L, int level, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  ncluax_print_error (L, level, "NCLUA ERROR\t", format, args);
  va_end (args);
}

/* Outputs Lua warning at level LEVEL to standard error.  */

void
ncluax_warning (lua_State *L, int level, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  ncluax_print_error (L, level, "NCLUA Warning\t", format, args);
  va_end (args);
}

/* Do a shallow dump of value at index INDEX to standard error.  */

static void
ncluax_dump_value (lua_State *L, int index)
{
  index = ncluax_abs (L, index);
  switch (lua_type (L, index))
    {
    case LUA_TBOOLEAN:
      fputs (lua_toboolean (L, index) ? "true" : "false", stderr);
      break;
    case LUA_TNUMBER:
      fprintf (stderr, "%g", lua_tonumber (L, index));
      break;
    case LUA_TNIL:
    case LUA_TNONE:
      fputs ("<nil>", stderr);
      break;
    case LUA_TSTRING:
      fprintf (stderr, "'%s'", lua_tostring (L, index));
      break;
    case LUA_TFUNCTION:
    case LUA_TLIGHTUSERDATA:
    case LUA_TTABLE:
    case LUA_TTHREAD:
    case LUA_TUSERDATA:
      fprintf (stderr, "<%s:%p>", lua_typename (L, lua_type (L, index)),
               lua_topointer (L, index));
      break;
    default:
      ASSERT_NOT_REACHED;
    }
}

/* Dumps the contents of table at INDEX to standard error.

   The DEPTH parameter controls the maximum number of recursive calls
   permitted when dumping nested tables.  If DEPTH < 0, then this number is
   assumed to be unlimited; beware that this may case infinite loops.  */

void
ncluax_dump_table (lua_State *L, int index, int depth)
{
  nclua_bool_t first = TRUE;
  int t = ncluax_abs (L, index);

  fflush (stdout);
  fprintf (stderr, "<%s:%p:{", lua_typename (L, LUA_TTABLE),
           lua_topointer (L, t));

  lua_pushnil (L);
  while (lua_next (L, t) != 0)
    {
      if (likely (!first))
        fputc (',', stderr);
      else
        first = FALSE;

      ncluax_dump_value (L, -2);
      fputc ('=', stderr);

      if (lua_type (L, -1) == LUA_TTABLE && depth != 0)
        ncluax_dump_table (L, -1, depth - 1);
      else
        ncluax_dump_value (L, -1);

      lua_pop (L, 1);
    }

  fputs ("}>", stderr);
  fflush (stderr);
}

/* Dumps the contents of the Lua stack to standard error.

   The DEPTH parameter controls the maximum number of recursive calls
   permitted when dumping nested tables.  If DEPTH < 0, then this number is
   assumed to be unlimited; beware that this may case infinite loops.  */

void
ncluax_dump_stack (lua_State *L, int depth)
{
  int i;
  int n;

  fflush (stdout);
  fprintf (stderr, "NCLua stack dump (%p):\n", (void *) L);

  n = lua_gettop (L);
  for (i = n; i >= 1; i--)
    {
      fprintf (stderr, "#%d\t", i);

      if (lua_type (L, i) == LUA_TTABLE && depth != 0)
        ncluax_dump_table (L, i, depth);
      else
        ncluax_dump_value (L, i);

      fputc ('\n', stderr);
    }

  fflush (stderr);
}