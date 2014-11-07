--CrawlHelpers = require('rcs/crawl_helpers.lua')

CrawlState = {
  enemies = function()
    local et = {}
    crawl_helpers.sight_range_iterator(function(x,y)
      m = monster.get_monster_at(x,y)
      if (m) then
        table.insert(et, {enemy = m, x = x, y = y})
      end
    end)
    return et
  end
}

return CrawlState
