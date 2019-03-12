/* Copyright (C) 2006-2018 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <https://www.gnu.org/licenses/>.  */

#include "LuaAPI.h"
#include "Player.h"

void
LuaAPI::Player_attachWrapper (lua_State *L, Player *player, Media *media)
{
  static const struct luaL_Reg *const funcs[] =
    {
     _funcs,
     NULL,
    };

  static const Chunk *const chunks[] =
    {
     &LuaAPI::_initMt,
     &LuaAPI::_Player_initMt,
     &LuaAPI::_traceMt,
     NULL,
    };

  Player **wrapper;

  g_return_if_fail (L != NULL);
  g_return_if_fail (player != NULL);
  g_return_if_fail (media != NULL);

  // Load and initialize metatable, if not loaded yet.
  LuaAPI::_loadLuaWrapperMt (L, LuaAPI::_PLAYER, funcs, chunks);

  // Create wrapper for event.
  wrapper = (Player **) lua_newuserdata (L, sizeof (Player **));
  g_assert_nonnull (wrapper);
  *wrapper = player;
  luaL_setmetatable (L, LuaAPI::_PLAYER);

  // Set LUA_REGISTRY[evt]=wrapper.
  LuaAPI::_attachLuaWrapper (L, player);

  // Call evt:_attachData().
  LuaAPI::Media_push (L, media);
  LuaAPI::_callLuaWrapper (L, player, "_attachData", 1, 0);
}

void
LuaAPI::Player_detachWrapper (lua_State *L, Player *player)
{
  g_return_if_fail (L != NULL);
  g_return_if_fail (player != NULL);

  // Call evt:_detachData().
  LuaAPI::_callLuaWrapper (L, player, "_detachData", 0, 0);

  // Set LUA_REGISTRY[evt] = nil.
  LuaAPI::_detachLuaWrapper (L, player);
}

Player *
LuaAPI::Player_check (lua_State *L, int i)
{
  g_return_val_if_fail (L != NULL, NULL);
  return *((Player **) luaL_checkudata (L, i, LuaAPI::_PLAYER));
}

void
LuaAPI::Player_push (lua_State *L, Player *player)
{
  g_return_if_fail (L != NULL);
  g_return_if_fail (player != NULL);

  LuaAPI::_pushLuaWrapper (L, player);
}

void
LuaAPI::Player_call (lua_State *L, Player *player, const char *name,
                     int nargs, int nresults)
{
  g_return_if_fail (L != NULL);
  g_return_if_fail (player != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (nargs >= 0);
  g_return_if_fail (nresults >= 0);

  LuaAPI::_callLuaWrapper (L, player, name, nargs, nresults);
}