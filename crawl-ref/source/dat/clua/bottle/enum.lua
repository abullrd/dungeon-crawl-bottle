-- TODO move to c, use metatable method to prevent modification

enum = {
  mon_attitude_type = {
    ATT_HOSTILE = 0,                       -- 0, default in most cases
    ATT_NEUTRAL = 1,                       -- neutral
    ATT_STRICT_NEUTRAL = 2,                -- neutral, won't attack player. Used by Jiyva.
    ATT_GOOD_NEUTRAL = 3,                  -- neutral, but won't attack friendlies
    ATT_FRIENDLY = 4,                      -- created friendly (or tamed?)
  }
}

return enum
