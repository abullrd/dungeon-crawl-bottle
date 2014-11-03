processing = false

-- get target travel square
-- eval moves based on whether we get closer to target

function command(input)
  crawl.delay(100)
  crawl.flush_input()
  crawl.sendkeys(input.. string.char(27) .. string.char(27) 
      .. string.char(27))
end

function delta_to_vi(dx, dy)
 local d2v = {
   [-1] = { [-1] = 'y', [0] = 'h', [1] = 'b'},
   [0]  = { [-1] = 'k', [0] = 's', [1] = 'j'},
   [1]  = { [-1] = 'u', [0] = 'l', [1] = 'n'},
 }
 return d2v[dx][dy]
end

function healthPercent()
  local hp, hp_max = you.hp()
  return tonumber(hp)/tonumber(hp_max)
end

GOAL_HEAL, GOAL_FIGHT, GOAL_EXPLORE, GOAL_NEXT_LEVEL= 1, 2, 3, 4
POTION_HEAL = 1


HUNGER_STARVING = "starving"
--, "near starving", "very hungry",
 --   "hungry",
  --  "not hungry",
   -- "full",
    --"very full",
    --"completely stuffed",

function havePotion(potion)
  return false
end

function rest()
  if not you.feel_safe() or you.hunger_name == HUNGER_STARVING then
    return false
  end

  command('5')
  return true
end

--function get_enemy(x, y)
--  --crawl.mpr("TODO check if candidate for attack")
--  return monster.get_monster_at(x,y)
--end
--
--function get_nearby_enemies()
--  local x, y
--  local monsters = {}
--  bestx = 0
--  besty = 0
--  best_info = nil
--  for x = -8,8 do
--    for y = -8,8 do
--      local monst = get_enemy(x, y)
--      if monst then
--        table.insert(monsters, monst)
--      end
--    end
--  end
--  return monsters
--end
--
--TODO this is so dumb
function is_explored()
  local count = 0
  local x_1, x_2 = map.x_bounds()
  local y_1, y_2 = map.y_bounds()
  for m_x = x_1, x_2 do
    for m_y = y_1, y_2 do
      if (current_state().map[m_x][m_y] == 1) then
        count = count + 1
      end
    end
  end

  return count == 0
end

function mark_seen()
  local p_x,p_y = map.player_pos()
  for x = -8, 8 do
    for y = -8, 8 do
      if map.in_bounds(p_x + x, p_y + y) and map.seen(p_x + x, p_y + y) then
        current_state().map[p_x +x][p_y + y] = 2
      end
    end
  end
end

function get_goal()
  local p_x, p_y = map.player_pos()
  mark_seen()
  check_surroundings()

  if #current_state().enemies > 0 then
    if healthPercent() < 0.20 and havePotion(POTION_HEAL) then
      return GOAL_HEAL
    end
    return GOAL_FIGHT
  end

  if healthPercent() < 0.75 then
    return GOAL_HEAL
  end

  if is_explored() then
    return GOAL_NEXT_LEVEL
  end

  local target = current_state().explore_target

  if (not target or (target.x == p_x and target.y == p_y)) then
    local exp_x, exp_y = find_explore_target()
    current_state().explore_target = { x = exp_x, y = exp_y }
  end

  return GOAL_EXPLORE
end

function make_a_move(goal)
  if goal == GOAL_HEAL then
    if you.feel_safe() then
      rest()
    end
  elseif goal == GOAL_FIGHT then
    hit_closest()
    crawl.sendkeys(string.char(27))
    crawl.flush_input()
  elseif goal == GOAL_EXPLORE then
    --travel.explore()
    local xx = current_state().explore_target.x
    local yy = current_state().explore_target.y
    crawl.mpr("Explore target " .. xx .. " " .. yy)
    travel.travel_towards(xx,yy)

  elseif goal == GOAL_NEXT_LEVEL then
    command("G>")
  end
end

function autobot()
  ready()
end

function autobot_interrupt_macro(interrupt_name)
  processing = true
  return false 
end

chk_interrupt_macro.autobot = autobot_interrupt_macro

function ch_stop_running(s)
  crawl.mpr("stop running\n")
end

function ch_start_running(s)
  crawl.mpr("start running\n")
  command("")
end

--function get_map()
--  thislevel = current_state.map[you.where]
--  if not thislevel then
--    thislevel = dgn_map[you.where] = {}
--    x_1, x_2 = map.x_bounds()
--    for (local i=x_1, x_2; i++) do
--
--    end
--    --dgn_map[you.where][
--  end
--  return thislevel
--end

global_state = {}

function current_state()
  if global_state[you.where] then
    return global_state[you.where]
  end

  local lvl_map = {}
  local m_x, m_y
  local x_1, x_2 = map.x_bounds()
  local y_1, y_2 = map.y_bounds()
  for m_x = x_1, x_2 do
    lvl_map[m_x] = {}
    for m_y = y_1, y_2 do
      lvl_map[m_x][m_y] = 0
    end
  end

  global_state[you.where] = {
    enemies = {},
    map = lvl_map
  }

  return global_state[you.where]
end

function check_surroundings()
  local p_x,p_y = map.player_pos()
  crawl.mpr("Current location " .. p_x .. " " .. p_y)
  current_state().enemies = {}

  local x,y
  for x = -8, 8 do
    for y = -8, 8 do
      m = monster.get_monster_at(x,y)
      if m then
        table.insert(current_state().enemies, {enemy = m, x = x, y = y})
      end
      -- if item

      local feature = view.feature_at(x,y)
      if map.in_bounds(p_x + x, p_y + y) and map.seen(p_x + x, p_y + y) and feature and travel.feature_traversable(feature) then
        local dx, dy
        for dx= -1,1 do
          for dy= -1,1 do
            if map.in_bounds(p_x + x + dx, p_y + y + dy) then
              if not map.seen(p_x + x + dx, p_y + y + dy) then
                current_state().map[p_x + x][p_y + y] = 1
              --else
               -- current_state().map[p_x + x][p_y + y] = 1
              end
            end
          end
        end
      end
    end
  end
end

-- TODO yeah this does way too much work
function find_explore_target()
  local lvl_map = current_state().map
  local p_x, p_y = map.player_pos()
  local x_1, x_2 = map.x_bounds()
  local y_1, y_2 = map.y_bounds()
  for dist = 1, math.max(x_2, y_2) do
    for i = p_x - dist, p_x + dist do
      for k = p_y - dist, p_y + dist do
        if k ~= p_y or i ~= p_x then
          if map.in_bounds(i, k) and lvl_map[i][k] == 1 then
            return i,k
          end
        end
      end
    end
  end
  crawl.mpr("ERROR : explore target not found\n")
  return nil
end


function printExplored()
  local count = 0
  local count2 = 0
  local count3 = 0
  local x_1, x_2 = map.x_bounds()
  local y_1, y_2 = map.y_bounds()
  for m_x = x_1, x_2 do
    for m_y = y_1, y_2 do
      if (current_state().map[m_x][m_y] == 1) then
        count = count + 1
      end
      if (current_state().map[m_x][m_y] == 2) then
        count2 = count2 + 1
      end
      if (current_state().map[m_x][m_y] == 0) then
        count3 = count3 + 1
      end
    end
  end
  crawl.mpr("Explored count " .. count2 .. " toexplore " .. count .. " unknown " .. count3.."\n")
end

function ready()
  crawl.more_autoclear(true)
  goal = get_goal()
  crawl.mpr("GOAL : " .. tostring(goal) .. "\n")
  printExplored()
  make_a_move(goal)
end
