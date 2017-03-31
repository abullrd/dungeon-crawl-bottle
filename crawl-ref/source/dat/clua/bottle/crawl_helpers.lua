local CrawlHelpers = {}

CrawlHelpers.in_bounds = function(x,y)
  local x_1, x_2 = map.x_bounds()
  local y_1, y_2 = map.y_bounds()

  return x >= x_1 and
         x <= x_2 and
         y >= y_1 and
         y <= y_2
end

CrawlHelpers.box_iterator = function(size, center, fn)
  for i = center.x - size, center.x + size do
    for j = center.y - size, center.y + size do
      fn(i,j)
    end
  end
end

CrawlHelpers.map_box_iterator = function(size, center, fn)
  CrawlHelpers.box_iterator(size, center, function(x,y)
    if (CrawlHelpers.in_bounds(x,y)) then
      fn(x,y)
    end
  end)
end

CrawlHelpers.sight_range_iterator = function(fn)
  CrawlHelpers.box_iterator(8, {x = 0, y = 0 }, function(x,y)
    local p_x, p_y = map.player_pos()
    if (CrawlHelpers.in_bounds(p_x + x, p_y + y)) then
      fn(x,y)
    end
  end)
end

CrawlHelpers.map_sight_range_iterator = function(fn)
  local x, y = map.player_pos()
  CrawlHelpers.map_box_iterator(8, {x = x, y = y}, fn)
end

CrawlHelpers.map_iterator = function(fn)
  local x_1, x_2 = map.x_bounds()
  local y_1, y_2 = map.y_bounds()

  for x = x_1, x_2 do
    for y = y_1, y_2 do
      fn(x,y)
    end
  end
end

-- We can do better
CrawlHelpers.bfs = function(search_map, fn, candidate_fn)
  local p_x, p_y = map.player_pos()

  -- reset
  CrawlHelpers.map_iterator(function(x,y)
    search_map[x][y].bfs_marked = false
  end)

  local bfs_queue = Queue.new()
  bfs_queue:pushback({x = p_x, y = p_y})

  while(bfs_queue:peek()) do
    local coord = bfs_queue:pop()

    if (not search_map[coord.x][coord.y].bfs_marked) then
      if fn(coord.x, coord.y) then
        return coord.x, coord.y
      end

      search_map[coord.x][coord.y].bfs_marked = true
      CrawlHelpers.map_box_iterator(1, coord, function(x,y)
          if (not search_map[x][y].bfs_marked and candidate_fn(x,y)) then
            bfs_queue:pushback({ x = x, y = y })
          end
      end)
    end
  end

  return nil
end


return CrawlHelpers
