#include "AppHdr.h"

#include "l_libs.h"
#include "l_defs.h"

#include "cluautil.h"
#include "cloud.h"
#include "coord.h"
#include "env.h"
#include "mon-death.h"
#include "player.h"
#include "religion.h"
#include "terrain.h"
#include "travel.h"
#include "stringutil.h"
#include "view.h"


LUARET2(map_x_bounds, number, X_BOUND_1, X_BOUND_2);

LUARET2(map_y_bounds, number, Y_BOUND_1, Y_BOUND_2);

LUAFN(map_in_bounds)
{
  coord_def c;
  c.x = luaL_checkint(ls, 1);
  c.y = luaL_checkint(ls, 2);
  PLUARET(boolean, in_bounds(c));
  return 1;
}

LUAFN(map_seen)
{
    COORDS(s, 1, 2)
    if (!map_bounds(s))
    {
        PLUARET(boolean, false);
        return 1;
    }
    PLUARET(boolean, env.map_knowledge(s).seen());
    return 1;
}

LUARET2(map_player_pos, number, you.pos().x, you.pos().y)

// Pushes the current map onto the stack
// Adds one item to the stack
void push_current_map(lua_State *ls)
{
  const char *branch_name = level_id::current().describe(false, false).c_str();
  char g_str[64];
  snprintf(g_str, 64, "map.knowledge.%s", branch_name);
  clua.pushglobal(g_str);
  lua_pushinteger(ls, you.depth);
  lua_gettable(ls, -2);
  lua_insert(ls, -2);
  lua_pop(ls, 1);
}

// Pushes the coordinate table for the current map onto the stack.
// Adds one item to the stack
void push_map_coord(lua_State *ls, const coord_def pos) {
  push_current_map(ls);
  lua_pushinteger(ls, pos.x);
  lua_gettable(ls, -2);
  lua_pushinteger(ls, pos.y);
  lua_gettable(ls, -2);
  lua_insert(ls, -3);
  lua_pop(ls, 2);
}

void clua_map_knowledge_update_seen(lua_State *ls, const coord_def pos)
{
  push_map_coord(ls, pos);
  lua_pushboolean(ls, env.map_knowledge(pos).seen());
  lua_setfield(ls, -2, "seen");

  lua_pushboolean(ls, env.map_knowledge(pos).known());
  lua_setfield(ls, -2, "known");

  lua_pushfstring(ls, dungeon_feature_name(env.map_knowledge(pos).feat()));
  lua_setfield(ls, -2, "feature");

  lua_pop(ls, 1);
}

void initialize_level_map(lua_State *ls)
{
  coord_def c;
  lua_createtable(ls, X_BOUND_2 - X_BOUND_1, 0);
  for (int x = X_BOUND_1; x <= X_BOUND_2; ++x) {
    lua_createtable(ls, Y_BOUND_2 - Y_BOUND_1, 0);
    for (int y = Y_BOUND_1; y <= Y_BOUND_2; ++y) {
      c.x = x;
      c.y = y;

      lua_createtable(ls, 0, 5);

      lua_pushboolean(ls, env.map_knowledge(c).seen());
      lua_setfield(ls, -2, "seen");

      lua_pushboolean(ls, env.map_knowledge(c).known());
      lua_setfield(ls, -2, "known");

      lua_pushboolean(ls, env.map_knowledge(c).detected_monster());
      lua_setfield(ls, -2, "detected_monster");

      lua_pushboolean(ls, env.map_knowledge(c).detected_item());
      lua_setfield(ls, -2, "detected_item");

      lua_pushfstring(ls, dungeon_feature_name(env.map_knowledge(pos).feat()));
      lua_setfield(ls, -2, "feature");

      lua_rawseti(ls, -2, y);
    }
    lua_rawseti(ls, -2, x);
  }
}

void clua_map_knowledge_load_level(lua_State *ls)
{
  clua.pushglobal("map.knowledge");
  const char *branch_name = level_id::current().describe(false, false).c_str();
  lua_getfield(ls, 1, branch_name);

  // If no branch table exists, create it and push onto stack
  if (lua_isnil(ls, -1)) {
    lua_pop(ls, 1);
    lua_createtable(ls, 20, 0);
  }

  // Get depth table
  lua_pushnumber(ls, you.depth);
  lua_gettable(ls, -2);

  //// If no depth table exists, create it and initialize
  if (lua_isnil(ls, -1)) {
    lua_pop(ls, 1);
    initialize_level_map(ls);
  }

  // Set depth table
  lua_rawseti(ls, -2, you.depth);

  // Set branch table
  lua_setfield(ls, -2, branch_name);

  // Pop map.knowledge
  lua_pop(ls, 1);
}

static void init_map(lua_State *ls) {
  lua_createtable(ls, 0, 30);
  lua_setfield(ls, -2, "knowledge");
}

static const struct luaL_reg map_lib[] =
{
    { "x_bounds", map_x_bounds },
    { "y_bounds", map_y_bounds },
    { "in_bounds", map_in_bounds },
    { "seen", map_seen },
    { "player_pos", map_player_pos },

    { NULL, NULL }
};

void cluaopen_map(lua_State *ls)
{
    luaL_openlib(ls, "map", map_lib, 0);
    init_map(ls);
}
