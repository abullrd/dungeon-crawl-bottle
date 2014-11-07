crawl_helpers= assert(loadfile('rcs/crawl_helpers.lua'))()
crawl_state = assert(loadfile('rcs/crawl_state.lua'))()


WrapperState = {
  load_script = function()
    local f = assert(loadfile("rcs/userscript.lua"))
    WrapperState.usr_script = f()
  end,

  execute_script = function()
    WrapperState.load_script()
    crawl.mpr("Script executing")
    crawl.delay(100)
    crawl.more_autoclear(true)
    WrapperState.usr_script.start_turn()
    WrapperState.process_keys()
  end,

  process_keys = function()
    crawl.flush_input()
    if (#CrawlBot.commands == 0) then
      CrawlBot.queue_command('5')
    end

    local cmdStr = ""
    for i=1, #CrawlBot.commands do
      --cmdStr = cmdStr .. CrawlBot.commands[i]
      crawl.sendkeys(CrawlBot.commands[i])
    end
    CrawlBot.commands = {}
  end

}

local repeatScript = false

function ready()
  crawl.mpr("Turn Start")
  if (repeatScript) then
    run_script()
  end
end

function run_script()
    WrapperState.execute_script()
end

function toggle_script()
    repeatScript = not repeatScript
    if (repeatScript) then
      run_script()
      return
    end
    crawl.mpr("Script stop")
  end

CrawlBot = {
  queue_command = function(cmd)
    table.insert(CrawlBot.commands, cmd)
  end,
  commands = {}
}

