 local priority_monsters = {"ice beast","orc warrior","'s ghost",
 "Sigmund","Menkaure","hill giant","Pikel","war dog","orc priest",
 "orc wizard","Prince Ribbit","blink frog","Edmund","Nessos","Maud",
 "Grinder","Erolcha","Rupert","electric eel","Yiuf","ogre","troll",
 "Dowan","Duvessa","Nessos","giant eyeball","fire drake","dragon",
 "Sonja","berserk","spiny frog","Erica","Jozef","Harold","jelly","wasp"}
 local scary_monsters = {"hydra","statue","Nessos","Nikola","Sonja"}
 local hard_monsters = {"ice beast","orc warrior","'s ghost","Sigmund",
 "Menkaure","hill giant","Pikel","war dog","orc priest","Edmund",
 "Nessos","Maud","Erolcha","Rupert","goliath","electric eel","Yiuf",
 "ogre","troll","Duvessa","fire drake","dragon","berserk",
 "spiny frog","dire elephant","Erica","Jozef","Harold","jelly","red wasp"}
 local hand_monsters = {"Grinder","Erolcha","Rupert"}
 local no_flee_monsters = {"orc priest","Sigmund","electric eel",
 "Dowan","berserk","giant eyeball","centaur","Nessos","fire drake",
 "yaktaur","dragon","Blork","killer bee","Joseph","Erica","orc wizard"}
 local easy_monsters = {"green rat","porcupine","quokka","megabat","fleeing"}
 local resistance_monsters = {"fire drake","Nikola"}

 -- some variables:

 local potion_choice = {none}
 local hold = false
 local last_search = "none"
 local tries = 0
 local travel_goal = "corpses"
 local move = "none"
 local last_turn = -1
 local last_hand_turn = -100
 local last_hp = 1000
 local prev_hp = 1000
 local last_mhp = 1000
 local last_xl = 1
 local failed_rest = false

 local branch_goal = "Lair:1"
 local branch_command = "GL\r"

 local forbidden_stairs = "none"
 local random_down = false
 local last_where = you.where()
 local last_absdepth = you.absdepth()

 local stepped_on_lair = false

 local is_berserk = you.berserk()

 local dump_count = 0
 local death_count = 0

 local max_hunger = "not hungry"
 if you.race() == "Troll" or you.race() == "Kobold" 
    or you.race() == "Felid" then
   max_hunger = "completely stuffed"
 end

--checks for changes

 function update_stuff()
   if you.turns() >= 1000*dump_count then
     dump_count = dump_count+1
     magic('#')
   end

   update_branch_goal()

   local new_hp, new_mhp = you.hp()
   if death_count < you.deaths() then
     death_count = you.deaths()
     is_berserk = false
     say("Death " .. death_count .. " :/")
   end
   if is_berserk and not you.berserk() then
     say("Berserk ended")
     is_berserk = false
   elseif (not is_berserk) and you.berserk() then 
     is_berserk = true
   end

   if not (last_where == you.where()) then
     if random_down then
       random_down = false
       forbidden_stairs = "none"
     elseif move == "autotravel" and travel_goal == "prev level" 
          and you.absdepth() == last_absdepth-1 then
       random_down = true
       forbidden_stairs = view.feature_at(0,0)
     end
   end 
   last_where = you.where()
   last_absdepth = you.absdepth()

     failed_rest = false

   if last_turn == you.turns() then
     if move == "rest" then
       failed_rest = true
     end
     tries = tries + 1
   else
     tries = 0
     travel_goal = "corpses"
     last_turn = you.turns()
     prev_hp = last_hp
   end
   last_hp = new_hp
   last_mhp = new_mhp
   last_xl = you.xl()
 end

-- the main decision-making function and four subfunctions
-- tactical means at least one non-is_safe monster is in sight

 function choose_move()
   if c_monsters(8) > 0 then
     if is_berserk then
       choose_berserk_tactical()
     else
       choose_calm_tactical()
     end
   else
     if is_berserk then
       choose_berserk_not_tactical()
     else
       choose_calm_not_tactical()
     end
   end
 end

 -- this one is simple :)
 function choose_berserk_not_tactical()
   move = "wait"
 end

 function choose_berserk_tactical()
   if c_bia(2) > 0 and last_hp*4 < last_mhp and is_good_retreat() then
     move = "flee"
   elseif is_reasonable_attack() then
     move = "attack"
   else
     move = "flee"
   end
 end

 --should add in item use here, and also figure out about glass walls...
 --assess_danger() is an important auxiliary function here
 function choose_calm_tactical()
   if (you.confused() or you.rotting()) and have_potion("healing") then
     move = "drink potion"
     potion_choice = "healing"
     return
   end
   danger = assess_danger()
   if danger >= 2 then
     if c_bia(4) > 0 then
       if can_berserk() then
         move = "berserk"
       else
         move = "flee"
       end
     else
       if can_bia() then
         move = "bia"
       elseif is_good_retreat() then
         move = "flee"
       elseif can_berserk() then
         move = "berserk"
       else
         move = "attack"
       end
     end
   elseif danger >= 1 then
     if c_bia(4) > 0 then
       if can_berserk() then
         move = "berserk"
       else
         move = "flee"
       end
     elseif can_hand() and c_spec_monsters(8,hand_monsters) > 0 then
       move = "hand"
     elseif can_berserk() then
       move = "berserk"
     elseif can_bia() then
       move = "bia"
     elseif is_good_retreat() then
       move = "flee"
     else
       move = "attack"
     end
   else
     if can_hand() and c_spec_monsters(8,hand_monsters) > 0 then
       move = "hand"
     elseif you.exhausted() and (monster_distance()>1) 
         and is_good_retreat() then
       move = "flee"
     else
       move = "attack"
     end
   end
   if move == "flee" and c_monsters(1)==0 
       and last_hp*5<last_mhp and can_hand() and not you.sick() then
     move = "hand"
   end   
 end

 --should add in better travel here
 function choose_calm_not_tactical()  
   if you.rotting() and have_potion("healing") then
     move = "drink potion"
     potion_choice = "healing"
   elseif you.poisoned() and last_hp<5 then
     if have_potion("healing") then
       move = "drink potion"
       potion_choice = "healing"
     elseif can_hand() and not you.sick() then
       move = "hand"
     elseif have_potion("heal wounds") then
       move = "drink potion"
       potion_choice = "heal wounds"
     elseif have_unknown_potion() then
       move = "try potion"
     else
       move = "rest"
     end
   elseif (last_hp < 3 or last_hp*10 < last_mhp) and can_hand() 
         and not you.sick() then
     move = "hand"
   elseif should_restore() then
     move = "drink potion"
     potion_choice = "restore abilities"
   elseif have_potion("gain") then
     move = "drink potion"
     potion_choice = "gain"
   elseif have_potion("experience") then
     move = "drink potion"
     potion_choice = "experience"
   elseif have_potion("of mutation") then
     move = "drink potion"
     potion_choice = "of mutation"
   elseif have_chunks() and not (you.hunger() == max_hunger) then
     move = "eat chunks"
   elseif (you.hunger() == "very hungry" or you.hunger() == "near starving" 
       or you.hunger() == "starving") and have_permafood() then
     move = "eat permafood"
   elseif last_hp < last_mhp or you.confused() then
     move = "rest"
   elseif is_reasonable_ballisto_attack() then
     move = "ballisto attack" 
   elseif on_corpses() then
     if (you.hunger() == max_hunger 
         or on_dangerous_corpse() or you.levitating()) 
         and not string.find(view.feature_at(0,0),"altar") then
       move = "pray"
     else
       move = "chop"
     end
   elseif you.exhausted() then
     move = "rest"
   elseif branch_goal == "Lair:1" and can_see_lair() then
     move = "step towards Lair"
   else
     if tries >= 1 then
       if travel_goal == "corpses" then
         travel_goal = "autoexplore"
       elseif travel_goal == "autoexplore" then
         travel_goal = "branch"
         if branch_command == "none" then
           travel_goal = "next level"
         end
       elseif travel_goal == "branch" then
         travel_goal = "next level"
       elseif travel_goal == "next level" then
         travel_goal = "prev level"
       elseif travel_goal == "prev level" then
         travel_goal = "stuck"
       end
        tries = 0
     end
     move = "autotravel"
     if travel_goal == "next level" then
       if have_bad_amulet() then
         move = "drop bad amulet"
       elseif have_amulet() then
         move = "wear amulet"
       elseif have_ring() then
         move = "wear ring"
       elseif have_unknown_potion() then
         move = "try potion"
       end
     end
   end
 end

-- measures the danger of the monsters around, in a very crude way
 

 function assess_danger()
   if (c_ignore_spec_monsters(8,easy_monsters) > 6 and c_monsters(4) > 0) 
      or c_spec_monsters(4, scary_monsters) > 0 then
     return 2
   elseif (c_monsters(2) > 0 and c_monsters(8) > 3) 
 or (c_ignore_spec_monsters(4,easy_monsters) > 0 and last_hp*2 < last_mhp) 
 or c_spec_monsters(2, hard_monsters) > 0 then
     return 1
   else 
  return 0
   end
 end

-- function that actually executes the move

 function make_move()
   if move == "rest" then
     if failed_rest then
       magic('s')
     else
       magic('5')
     end
   elseif move == "flee" then
     flee()
   elseif move == "attack" then
     make_attack(get_target())
   elseif move == "berserk" then
     say("Berserk")
     magic('aa')
   elseif move == "bia" then
     say("Attempting Brothers in Arms")
     magic('ac')
   elseif move == "hand" then
     last_hand_turn = you.turns()
     say("Trog's Hand")
     magic('ab')
   elseif move == "eat chunks" then
     magic('ee')
   elseif move == "eat permafood" then
     say("Eating permafood")
     eat_permafood()
   elseif move == "ballisto attack" then
     make_attack(get_ballisto_target())
   elseif move == "pray" then
     magic('p')
   elseif move == "chop" then
     magic('ccq')
   elseif move == "wait" then
     magic('s')
   elseif move == "step towards Lair" then
     step_towards_lair()
   elseif move == "autotravel" then
     autotravel()
   elseif move == "drop bad amulet" then
     drop_bad_amulet()
   elseif move == "try potion" then
     try_potion()
   elseif move == "wear amulet" then
     wear_amulet()
   elseif move == "wear ring" then
     wear_ring()
   elseif move == "drink potion" then
     drink_potion(potion_choice)
   else
     say("bad move")
   end
 end

-- travel is tricky

 function autotravel()
   if travel_goal == "corpses" then
     if last_search == "corpses" then
       magic('*f\ra\r')
     else
       last_search = "corpses"
       magic('*fcorpse&&!!rot&&!!skelet\ra\r')
     end
   elseif travel_goal == "autoexplore" then
     magic('o')
   elseif travel_goal == "branch" and not (branch_command == "none") then
     magic(branch_command)
   elseif travel_goal == "next level" then
     if random_down then
       head_to_random_down()
     else
       magic('G>')
     end
   elseif travel_goal == "prev level" then
     magic('G<')
   elseif travel_goal == "stuck" then
     make_random_move()
   else
     say("bad travel_goal")
   end
 end

 function head_to_random_down()
   local i,j
   local c = "X"
   local feat = view.feature_at(0,0)
   if string.find(feat,"stone_stairs_down") then 
     if feat == forbidden_stairs then
       magic("X>\r")
     else
       magic('>')
     end
   else
     j = crawl.roll_dice(1,12)
     for i = 1,j do
       c = (c .. ">")
     end
     magic(c .. "\r")
   end
 end

 function next_branch_goal()
   if branch_goal == "Lair:1" then
     branch_goal = "Lair:7"
     branch_command = "none"
   elseif branch_goal == "Lair:7" then
     branch_goal = "back to D for Orc"
     branch_command = "GD\r"
   elseif branch_goal == "back to D for Orc" then
     branch_goal = "Orc:1"
     branch_command = "GO\r"
   elseif branch_goal == "Orc:1" then
     branch_goal = "Orc:3"
     branch_command = "none"
   elseif branch_goal == "Orc:3" then
     branch_goal = "back to D for Vaults"
     branch_command = "GD\r"
   elseif branch_goal == "back to D for Vaults" then
     branch_goal = "Vault:1"
     branch_command = "GV\r"
   elseif branch_goal == "Vault:1" then
     branch_goal = "Vault:2"
     branch_command = "none"
   elseif branch_goal == "Vault:2" then
     branch_goal = "Snake:1"
     branch_command = "GP\r"
   elseif branch_goal == "Snake:1" then
     branch_goal = "Snake:5"
     branch_command = "none"
   elseif branch_goal == "Snake:5" then
     branch_goal = "back to D for D:27"
     branch_command = "GD\r"
   elseif branch_goal == "back to D for D:27" then
     branch_goal = "???"
     branch_command = "none"
   else
     branch_goal = "error"
     branch_command = "none"
   end
 end

 function update_branch_goal()
   local old_branch_goal = branch_goal
   if you.where() == branch_goal then
     next_branch_goal()
   elseif you.branch() == "D" 
    and string.find(branch_goal, "back to D for") then
     next_branch_goal()
   elseif branch_goal == "Lair:1" and you.branch() == "D"
    and you.absdepth() > 13 then
     branch_goal = "Orc:1"
     branch_command = "GO\r"
     say("Couldn't find Lair!")
   elseif branch_goal == "Orc:1" and you.branch() == "D"
    and you.absdepth() > 14 then
     branch_goal = "Vault:1"
     branch_command = "GV\r"
     say("Couldn't find Orc!")
   elseif branch_goal == "Vault:1" and you.branch() == "D"
    and you.absdepth() > 19 then
     branch_goal = "???"
     branch_command = "none"
     say("Couldn't find Vault!")
   end
   if not (branch_goal == old_branch_goal) then
     branch_goal_found = false
     say("Next: " .. branch_goal)
   end
 end

-- now we are on to miscellaneous functions, 
-- only a few of which are complicated

 function move_towards(dx, dy)
   local move = nil
   if adjacent(dx, dy) then
     move = delta_to_vi(dx, dy)
   else
     move = try_move(sign(dx), sign(dy))
     if abs(dx) > abs(dy) then
       if move == nil then move = try_move(sign(dx),0) end
       if move == nil then move = try_move(0,sign(dy)) end
     else
       if move == nil then move = try_move(0,sign(dy)) end
       if move == nil then move = try_move(sign(dx),0) end
     end
   end
   return move
 end

 function make_attack(x, y)
   local move = move_towards(x, y)
   if move == nil then
     make_random_move()
   else
     magic(move)
   end
 end

 function get_target()
   local r = monster_distance()
   if r == 10 then
     return 0, 0
   end
   local x, y
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m and contains_string_in(m:desc(),priority_monsters)
           and not m:is_safe() and you.see_cell_no_trans(x,y) then
           return x, y
         end
       end
     end
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m and not m:is_safe() and you.see_cell_no_trans(x,y) then
           return x, y
         end
       end
     end
   return 0, 0
 end

 function get_ballisto_target()
   local r, x, y
   for r = 1,8 do
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m then
           if string.find(m:desc(), "ballisto")
             or string.find(m:desc(), "snail")
             or string.find(m:desc(), "snapping") then
             if you.see_cell_no_trans(x,y) then
               return x,y
             end
           end
         end
       end
     end
   end
   return 0,0
 end

  function flee()
  local x,y
  local i,j
  local best_move = nil
  local best_score = -1
  local best_adjacent = 1000
  local score
  local num_adjacent
  local m
  for i = -1,1 do
  for j = -1,1 do
  if can_move(i, j) and 
    not monster.get_monster_at(i,j) then
  score = 0
  num_adjacent = 0
  for x = -8,8 do
  for y = -8,8 do
    m = monster.get_monster_at(x, y)
      if m and not m:is_safe() then
        score = score + abs(x-i) + abs(y-j)
        if adjacent(x-i,y-j) then
          num_adjacent = num_adjacent+1
        end
      end
  end
  end
  if num_adjacent < best_adjacent 
    or (num_adjacent == best_adjacent and score > best_score) then
     best_score = score
     best_adjacent = num_adjacent
     best_move = delta_to_vi(i, j)
  end
  end
  end
  end
  if best_move == 's' then
    if monster_distance() == 1 then
      make_attack(get_target())
    else      
      make_random_move()
    end
  else
    say("FLEEEEING")
    magic(best_move)
  end
end

 --these functions need to be written
 function is_reasonable_attack()
   return true
 end

 function is_reasonable_ballisto_attack()
   x,y = get_ballisto_target()
   return not (x == 0 and y == 0)
 end

 function is_good_retreat()
   return (c_spec_monsters(5, no_flee_monsters) == 0)
 end

-- simple functions

 function monster_distance()
  local r, x, y
   for r = 1,8 do
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m and not m:is_safe() then
           return r
         end
       end
     end
   end
   return 10
 end

 function c_monsters(r)
   local x, y
   local i = 0
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m and not m:is_safe() then
           i = i+1
         end
       end
     end
   return i
 end

 function c_spec_monsters(r,t)
   local x, y
   local i = 0
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m and not m:is_safe() 
           and contains_string_in(m:desc(), t) then
           i = i+1
         end
       end
     end
   return i
 end

 function c_ignore_spec_monsters(r,t)
   local x, y
   local i = 0
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m and not m:is_safe() 
           and not contains_string_in(m:desc(), t) then
           i = i+1
         end
       end
     end
   return i
 end

 function c_bia(r)
   local x, y
   local i = 0
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m and m:is_safe() and string.find(m:desc(),"berserk") 
    and contains_string_in(m:desc(), {"ogre","giant","bear","troll"}) then
           i = i+1
         end
       end
     end
   return i
 end

 function can_move(x,y)
   return travel.feature_traversable(view.feature_at(x, y))
 end
 
 function try_move(dx, dy)
   if can_move(dx,dy) then
     return delta_to_vi(dx, dy)
   else
     return nil
   end
 end

 function make_random_move()
   local x = crawl.random2(3)-1
   local y = crawl.random2(3)-1
   if can_move(x,y) then
     magic(delta_to_vi(x, y))
   elseif x == 0 and y == 0 then
     magic('s')
   else
     make_random_move()
   end
 end

 function eat_permafood()
 local i
 local it
 local l
 for i = 0,51 do
   it = items.inslot(i)
   if it and contains_string_in(it.name(),{"ration","honey","jelly"}) then
     l = items.index_to_letter(i)
     magic('e' .. l)
     return
   end
 end
 end

 function on_corpses()
   local fl = you.floor_items()
   for it in iter.invent_iterator:new(fl) do
     if string.find(it.name(),"corpse") 
         and not string.find(it.name(),"rotting") then
       return true
     end
   end
   return false
 end

 function on_dangerous_corpse()
   local fl = you.floor_items()
   for it in iter.invent_iterator:new(fl) do
     if string.find(it.name(),"corpse") then
       return food.dangerous(it)
     end
   end
   return false
 end

 function have_chunks()
 local i
 local it
 for i = 0,51 do
   it = items.inslot(i)
   if it and string.find(it.name(),"chunk") and not food.dangerous(it) 
     and not food.rotting(it) then
     return true
   end
 end
 return false
 end

 function have_permafood()
 local i
 local it
 for i = 0,51 do
   it = items.inslot(i)
   if it and contains_string_in(it.name(),{"ration","honey","jelly"}) then
     return true
   end
 end
 return false
 end

 function can_hand()
   return (util.contains(you.abilities(),"Trog's Hand") 
      and last_hand_turn + 10 < you.turns()
      and (not (you.hunger() == "starving")))
      and not you.confused()
 end

 function can_bia()
   return (util.contains(you.abilities(),"Brothers in Arms") 
      and (not (you.hunger() == "starving")) and not you.confused())
 end

 function have_amulet()
 if items.equipped_at(9) then
   return false
 end
 local i
 for i = 0,51 do
   if items.inslot(i) and string.find(items.inslot(i).name(),"amulet") then
     return true
   end
 end
 return false
 end

 function wear_amulet()
 local i
 local l
 for i = 0,51 do
   if items.inslot(i) and string.find(items.inslot(i).name(),"amulet") then
     l = items.index_to_letter(i)
     say("WEARING AMULET")
     magic('P' .. l)
     return
   end
 end
 end

 -- we assume that if just one ring is on, it is the left one
 function have_ring()
   local one_ring
   if items.equipped_at("Left Ring") then
     if items.equipped_at("Right Ring") then
       return false
     else
       one_ring = true
     end
   else
     one_ring = false
   end
   local i
   for i = 0,51 do
     if items.inslot(i) and string.find(items.inslot(i).name(),"ring") 
         and not (one_ring and items.inslot(i).name() == 
         items.equipped_at("Left Ring").name()) then
       return true
     end
   end
   return false
 end

 function wear_ring()
   local i
   local l
   for i = 0,51 do
     if items.inslot(i) and string.find(items.inslot(i).name(),"ring") 
         and not (items.equipped_at("Left Ring") 
         and items.inslot(i).name() == 
         items.equipped_at("Left Ring").name()) then
       l = items.index_to_letter(i)
       say("WEARING RING")
       magic('P' .. l)
       return
     end
   end
 end

 function try_potion()
 local i
 local l
 for i = 0,51 do
   if items.inslot(i) and string.find(items.inslot(i).name(),"potion") 
       and not (string.find(items.inslot(i).name(), "potion of") 
       or string.find(items.inslot(i).name(), "potions of")) then
     l = items.index_to_letter(i)
     magic('q' .. l)
     return
   end
 end
 end

 function have_unknown_potion()
 local i
 for i = 0,51 do
   if items.inslot(i) and string.find(items.inslot(i).name(),"potion") 
       and not (string.find(items.inslot(i).name(), "potion of") 
       or string.find(items.inslot(i).name(), "potions of")) then
     return true
   end
 end
 return false
 end

 function have_potion(name)
 local i
 for i = 0,51 do
   if items.inslot(i) and string.find(items.inslot(i).name(), name) then
    return true
   end
 end
 return false
 end

 function drink_potion(name)
 local i
 local l
 for i = 0,51 do
   if items.inslot(i) and string.find(items.inslot(i).name(), name) then
     l = items.index_to_letter(i)
     say("Drinking " .. items.inslot(i).name())
     magic('q' .. l)
     return
   end
 end
 end

 function drop_bad_amulet()
   local l
   local i
   for i=0,51 do
     if items.inslot(i) and 
      contains_string_in(items.inslot(i).name(),{"rage","stasis"})
      and not items.inslot(i).cursed then
       l = items.index_to_letter(i)
       say("DROPPING BAD AMULET")
       magic('d' .. l)
       return
     end
   end
 end

 function have_bad_amulet()
   local i
   for i=1,51 do
     if items.inslot(i) and
      contains_string_in(items.inslot(i).name(),{"rage","stasis"})
      and not items.inslot(i).cursed then
       return true
     end
   end
 end

 function should_restore()
   if not have_potion("restore abilities") then
     return false
   end
   local str, mstr = you.strength()
   local int, mint = you.intelligence()
   local dex, mdex = you.dexterity()
   return (str+1 < mstr or int+4 < mint or int*2 <= mint or dex+1 < mdex)
 end

 function need_restore()
   if not have_potion("restore abilities") then
     return false
   end
   local str, mstr = you.strength()
   local int, mint = you.intelligence()
   local dex, mdex = you.dexterity()
   return (str <= 0 or int <= 0 or dex <= 0)
 end

 function can_berserk()
   if you.exhausted() or you.hunger() == "very hungry" 
        or you.hunger() == "near starving" or you.hunger() == "starving" 
        or you.mesmerised() or you.confused() or wearing_stasis() then
     return false
   else
     return true
   end
 end

 function wearing_stasis()
   return (items.equipped_at(9) and 
      string.find(items.equipped_at(9).name(),"stasis"))
 end

 function can_see_lair()
   local x, y
   if stepped_on_lair then
     return false
   end
   for x = -8,8 do
     for y = -8,8 do
       if view.feature_at(x,y) == "enter_lair"
           and you.see_cell_no_trans(x,y) then
         if x == 0 and y == 0 then
           stepped_on_lair = true
           return false
         else
           return true
         end
       end
     end
   end
   return false
 end

 function step_towards_lair()
   local x, y
   for x = -8,8 do
     for y = -8,8 do
       if view.feature_at(x,y) == "enter_lair" then
         make_attack(x,y)
         return
       end
     end
   end
 end

 function print_monsters(r)
   local x, y
     for x = -r,r do
       for y = -r,r do
         m = monster.get_monster_at(x, y)
         if m then
           say(m:desc())
           if m:is_safe() then
             say("=IS_SAFE ")
           end
           if not you.see_cell_no_trans(x,y) then
             say("=BEHIND_GLASS")
           end
         end
       end
     end
 end

 function print_behind_glass(r)
  local x, y
     for x = -r,r do
       for y = -r,r do
         if you.see_cell(x,y) and not you.see_cell_no_trans(x,y) then
           say(x .. "," .. y)
         end
       end
     end
 end

-- abstract functions

 function magic(command)
 hold = true
 crawl.delay(100)
 crawl.flush_input()
 crawl.sendkeys(command .. string.char(27) .. string.char(27) 
     .. string.char(27))
 coroutine.yield(true)
 hold = false
 crawl.flush_input()
 end

 function say(x)
   crawl.mpr(you.turns() .. " ||| " .. x)
   crawl.take_note(x)
 end

 function note(x)
   crawl.take_note(x)
 end

 function contains_string_in(name,t)
 for _, value in ipairs(t) do
 if string.find(name, value) then
 return true
 end
 end
 return false
 end

 function delta_to_vi(dx, dy)
   local d2v = {
     [-1] = { [-1] = 'y', [0] = 'h', [1] = 'b'},
     [0]  = { [-1] = 'k', [0] = 's', [1] = 'j'},
     [1]  = { [-1] = 'u', [0] = 'l', [1] = 'n'},
   }
   return d2v[dx][dy]
 end
 
 function sign(a)
   return a > 0 and 1 or a < 0 and -1 or 0
 end
 
 function abs(a)
   return a * sign(a)
 end
 
 function adjacent(dx, dy)
   return abs(dx) <= 1 and abs(dy) <= 1
 end

--macro the key d to ===s to use the script

 function s()
 if hold then
   return
 end
   update_stuff()
   choose_move()
   make_move()
 end

function s_interrupt_macro(interrupt_name)
  return false 
end

chk_interrupt_macro.s = s_interrupt_macro
