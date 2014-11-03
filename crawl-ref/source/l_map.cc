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


LUAFN(map_knowlege)
{
  lua_createtable(ls, Y_BOUND_2 - Y_BOUND_1, 0);
  for (int y = Y_BOUND_1; y <= Y_BOUND_2; ++y) {
    lua_createtable(ls, X_BOUND_2 - X_BOUND_1, 0);
    for (int x = X_BOUND_1; x <= X_BOUND_2; ++x) {
      coord_def c;
      c.x = x;
      c.y = y;

      lua_createtable(ls, Y_BOUND_2 - Y_BOUND_1, 3);

      lua_pushboolean(ls, env.map_knowledge(c).seen());
      lua_setfield(ls, -2, "seen");

      lua_pushboolean(ls, env.map_knowledge(c).detected_monster());
      lua_setfield(ls, -2, "detected_monster");

      lua_pushboolean(ls, env.map_knowledge(c).detected_item());
      lua_setfield(ls, -2, "detected_item");

      lua_rawseti(ls, -2, x);
    }
    lua_rawseti(ls, -2, y);
  }

  return 1;
}


static const struct luaL_reg map_lib[] =
{
    { "x_bounds", map_x_bounds },
    { "y_bounds", map_y_bounds },
    { "in_bounds", map_in_bounds },
    { "seen", map_seen },
    { "player_pos", map_player_pos },
    { "knowlege", map_knowlege },

    { NULL, NULL }
};

void cluaopen_map(lua_State *ls)
{
    luaL_openlib(ls, "map", map_lib, 0);
}
