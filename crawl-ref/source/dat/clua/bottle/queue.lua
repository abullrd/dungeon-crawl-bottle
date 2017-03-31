Queue = {}
Queue.__index = Queue

function Queue.new ()
  local q = {first = 0, last = -1}
  setmetatable(q, Queue)
  return q
end

function Queue.push(queue, elm)
  local new_first = queue.first - 1
  queue.first = new_first
  queue[new_first] = elm
end

function Queue.pushback(queue, elm)
  local new_last = queue.last + 1
  queue.last = new_last
  queue[new_last] = elm
end

function Queue.pop(queue)
  -- if queue.first > queue.last then error("list is empty") end
  local elm = queue[queue.first]
  queue[queue.first] = nil
  queue.first = queue.first + 1;
  return elm
end

function Queue.peek(queue)
  return queue[queue.first]
end

return Queue
