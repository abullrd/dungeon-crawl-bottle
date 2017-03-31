crawl_helpers = assert(loadfile('dat/clua/bottle/crawl_helpers.lua'))()
crawl_state = assert(loadfile('dat/clua/bottle/crawl_state.lua'))()
queue = assert(loadfile('dat/clua/bottle/queue.lua'))()
enum = assert(loadfile('dat/clua/bottle/enum.lua'))()


WrapperState = {
  load_script = function()
    local scriptfile
    if is_webtiles then
      scriptfile = "rcs/".. you.name() .. ".scripts/luascript/main.lua"
    else
      scriptfile = "dat/clua/bottle/autobot.lua"
    end

    local env = setmetatable({
      c_state = crawl_state,
      c_helpers = crawl_helpers,
      crawl = crawl,
      map = map,
      travel = travel,
      queue = queue,
      enum = enum
    }, {__index=_G})
    assert(pcall(setfenv(assert(loadfile(scriptfile)), env)))
    --setmetatable(env, nil)
    WrapperState.usr_script = env
  end,

  execute_script = function()
    WrapperState.load_script()
    --crawl.delay(100)
    crawl.more_autoclear(true)
    
    WrapperState.usr_script.start_turn(crawl_state)
    WrapperState.process_keys()
  end,

  process_keys = function()
    crawl.flush_input()
    if (#CrawlBot.commands == 0) then
      CrawlBot.queue_command('.')
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
  crawl.mpr("Turn Start\n")
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
    else
       crawl.mpr("Script stop\n")
       crawl.sendkeys('.')
    end
  end

CrawlBot = {
  queue_command = function(cmd)
    table.insert(CrawlBot.commands, cmd)
  end,
  commands = {}
}

