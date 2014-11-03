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
}
