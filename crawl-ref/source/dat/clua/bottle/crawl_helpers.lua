return {
  sight_range_iterator = function(fn)
    for x = -8, 8 do
      for y = -8, 8 do
        fn(x,y)
      end
    end
  end
}
