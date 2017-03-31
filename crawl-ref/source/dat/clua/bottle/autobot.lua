function current_map()
  return map.knowledge[you.branch()][you.depth()]
end

function explore()
   if #c_state.desired_items > 0 then
    --
  elseif current_map().explored then
    command('G>')
  else
    local x, y = c_helpers.bfs(current_map(), function(x,y)
      local target = false

      c_helpers.map_box_iterator(1, {x=x,y=y}, function(x1,y1)
        target = target or not current_map()[x1][y1].seen
      end)

      return target
    end, function(x,y)
      local square = current_map()[x][y]
      return square.feature and travel.feature_traversable(square.feature)
    end)

    if x then
      crawl.mpr("Target " .. x .. " " .. y)
      travel.travel_towards(x,y)
    else
      current_map().explored = true
      crawl.mpr("Exploration Complete")
    end
  end
end

function start_turn()
  local x, y = map.player_pos()

  if (#c_state.nearby_monsters({hostileOnly = true}) > 0) then
    crawl.mpr("Watch out!\n")
    hit_closest()
  else
    explore()
  end
end
