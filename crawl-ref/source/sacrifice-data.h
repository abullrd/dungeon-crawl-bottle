// See struct sacrifice_def in godabil.cc
static const sacrifice_def sac_data[] =
{

{ ABIL_RU_SACRIFICE_PURITY, MUT_NON_MUTATION,
  "corrupt yourself with",
  "sacrificed purity",

  0,
  SK_NONE,
  "current_purity_sacrifice",
},

{ ABIL_RU_SACRIFICE_WORDS, MUT_NO_READ,
  "your ability to read while threatened",
  "sacrificed words",

  28,
  SK_NONE,
  NULL,
},

{ ABIL_RU_SACRIFICE_DRINK, MUT_NO_DRINK,
  "your ability to drink while threatened",
  "sacrificed drink",

  28,
  SK_NONE,
  NULL,
},

{ ABIL_RU_SACRIFICE_ESSENCE, MUT_NON_MUTATION,
  "corrupt yourself with",
  "sacrificed essence",

  0,
  SK_NONE,
  "current_essence_sacrifice",
},

{ ABIL_RU_SACRIFICE_HEALTH, MUT_NON_MUTATION,
  "corrupt yourself with",
  "sacrificed health",

  20,
  SK_NONE,
  "current_health_sacrifice",
},

{ ABIL_RU_SACRIFICE_STEALTH, MUT_NO_STEALTH,
  "sacrifice your ability to go unnoticed",
  "sacrificed stealth",

  20,
  SK_STEALTH,
  NULL,
},

{ ABIL_RU_SACRIFICE_ARTIFICE, MUT_NO_ARTIFICE,
  "sacrifice all use of magical tools",
  "sacrificed evocations",

  60,
  SK_EVOCATIONS,
  NULL,
},

{ ABIL_RU_SACRIFICE_LOVE, MUT_NO_LOVE,
  "sacrifice your ability to be loved",
  "sacrificed love",

  20,
  SK_NONE,
  NULL,
},

{ ABIL_RU_SACRIFICE_COURAGE, MUT_COWARDICE,
  "sacrifice your courage",
  "sacrificed courage",

  25,
  SK_NONE,
  NULL,
},

{ ABIL_RU_SACRIFICE_ARCANA, MUT_NON_MUTATION,
  "Ru asks you to sacrifice all use of",
  "sacrificed arcana",

  25,
  SK_NONE,
  "current_arcane_sacrifices",
},

{ ABIL_RU_SACRIFICE_NIMBLENESS, MUT_NO_DODGING,
  "sacrifice your ability to dodge",
  "sacrificed dodging",

  20,
  SK_DODGING,
  NULL,
},

{ ABIL_RU_SACRIFICE_DURABILITY, MUT_NO_ARMOUR,
  "sacrifice your ability to wear armour well",
  "sacrificed armour",

  20,
  SK_ARMOUR,
  NULL,
},

{ ABIL_RU_SACRIFICE_HAND, MUT_MISSING_HAND,
  "sacrifice one of your",
  "sacrificed a hand",

  70,
  SK_SHIELDS,
  NULL,
},
};
