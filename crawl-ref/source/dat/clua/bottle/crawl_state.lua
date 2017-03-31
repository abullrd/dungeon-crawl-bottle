CrawlState = {
  nearby_monsters = function(args)
    local et = {}
    crawl_helpers.sight_range_iterator(function(x,y)
      m = monster.get_monster_at(x,y)
      if (m) then
        if (args.hostileOnly) then
          crawl.mpr("mon " ..m:name() .. " | "  .. m:attitude())
          crawl.mpr("mon " .. tostring(m:attitude() == enum.mon_attitude_type.ATT_HOSTILE))
        end
        table.insert(et, {monster = m, x = x, y = y})
      end
    end)
    return et
  end,

  desired_items = {}
}

return CrawlState
