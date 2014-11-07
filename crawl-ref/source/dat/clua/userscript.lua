
return {
  start_turn = function ()
    crawl.mpr("IN user script!")
    if (#crawl_state.enemies() == 0) then
      command('o')
    else
      crawl.mpr("Watch out")
      hit_closest()
    end
  end
}
