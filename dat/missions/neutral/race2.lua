--[[
<?xml version='1.0' encoding='utf8'?>
<mission name="Racing Skills 2">
 <avail>
  <priority>3</priority>
  <cond>player.pilot():ship():class() == "Yacht" and planet.cur():class() ~= "1" and planet.cur():class() ~= "2" and planet.cur():class() ~= "3" and system.cur():presences()["Independent"] ~= nil and system.cur():presences()["Independent"] &gt; 0</cond>
  <done>Racing Skills 1</done>
  <chance>20</chance>
  <location>Bar</location>
 </avail>
 <notes>
  <tier>2</tier>
 </notes>
</mission>
--]]
--[[
   --
   -- MISSION: Racing Skills 2
   -- DESCRIPTION: A person asks you to join a race, where you fly to various checkpoints and board them before landing back at the starting planet
   --
--]]

local fmt = require "format"

local chatter = {}
chatter[1] = _("Let's do this!")
chatter[2] = _("Wooo!")
chatter[3] = _("Time to Shake 'n Bake")
target = {1,1,1,1}

function create ()
   this_planet, this_system = planet.cur()
   missys = {this_system}
   if not misn.claim(missys) then
      misn.finish(false)
   end
   cursys = system.cur()
   curplanet = planet.cur()
   misn.setNPC(_("A laid back person"), "neutral/unique/laidback.webp", _("You see a laid back person, who appears to be one of the locals, looking around the bar, apparently in search of a suitable pilot."))
   credits_easy = rnd.rnd(20e3, 100e3)
   credits_hard = rnd.rnd(200e3, 300e3)
end


function accept ()
   if tk.yesno(_("Another race"), _([["Hey there, great to see you back! You want to have another race?"]])) then
      misn.accept()
      misn.setDesc(_("You're participating in another race!"))
      misn.osdCreate(_("Racing Skills 2"), {
         _("Board checkpoint 1"),
         _("Board checkpoint 2"),
         _("Board checkpoint 3"),
         fmt.f(_("Land at {pnt}"), {pnt=curplanet}),
      })
      local s = fmt.f(_([["There are two races you can participate in: an easy one, which is like the first race we had, or a hard one, with smaller checkpoints and no afterburners allowed. The easy one has a prize of {credits1}, and the hard one has a prize of {credits2}. Which one do you want to do?"]]), {credits1=fmt.credits(credits_easy), credits2=fmt.credits(credits_hard)})
      choice, choicetext = tk.choice(_("Choose difficulty"), s, _("Easy"), _("Hard"))
      if choice == 1 then
         credits = credits_easy
         tk.msg(_("Easy Mode "), _([["Let's go have some fun then!"]]))
      else
         credits = credits_hard
         tk.msg(_("Hard Mode"), _([["You want a challenge huh? Remember, no afterburners on your ship or you will not be allowed to race. Let's go have some fun!"]]))
      end
      misn.setReward(fmt.credits(credits))
      hook.takeoff("takeoff")
      else
      tk.msg(_("Refusal"), _([["I guess we'll need to find another pilot."]]))
   end
end

function takeoff()
   if player.pilot():ship():class() ~= "Yacht" then
      tk.msg(_("Illegal ship!"), _([["You have switched to a ship that's not allowed in this race. Mission failed."]]))
      misn.finish(false)
   end
   if choice ~= 1 then
      for k,v in ipairs(player.pilot():outfits()) do
         if v:type() == "Afterburner" then
            tk.msg(_("Illegal ship!"), _([["You have outfits on your ship which is not allowed in this race in hard mode. Mission failed."]]))
            misn.finish(false)
         end
      end
   end
   planetvec = planet.pos(curplanet)
   misn.osdActive(1)
   checkpoint = {}
   racers = {}
   pilot.toggleSpawn(false)
   pilot.clear()
   local srad = system.cur():radius()
   location1 = vec2.newP( srad * rnd.rnd(), 360*rnd.rnd() )
   location2 = vec2.newP( srad * rnd.rnd(), 360*rnd.rnd() )
   location3 = vec2.newP( srad * rnd.rnd(), 360*rnd.rnd() )
   if choice == 1 then
      shiptype = "Goddard"
   else
      shiptype = "Koala"
   end
   checkpoint[1] = pilot.add(shiptype, "Trader", location1, nil, {ai="stationary"})
   checkpoint[2] = pilot.add(shiptype, "Trader", location2, nil, {ai="stationary"})
   checkpoint[3] = pilot.add(shiptype, "Trader", location3, nil, {ai="stationary"})
   for i, j in ipairs(checkpoint) do
      j:rename(fmt.f(_("Checkpoint {n}"), {n=i}))
      j:setHilight(true)
      j:setInvincible(true)
      j:setActiveBoard(true)
      j:setVisible(true)
   end
   racers[1] = pilot.add("Llama", "Independent", curplanet)
   racers[2] = pilot.add("Gawain", "Independent", curplanet)
   racers[3] = pilot.add("Llama", "Independent", curplanet)
   if choice == 1 then
      racers[1]:outfitAdd("Engine Reroute")
      racers[2]:outfitAdd("Engine Reroute")
      racers[3]:outfitAdd("Improved Stabilizer")
   else
      for i in pairs(racers) do
         racers[i]:outfitRm("all")
         racers[i]:outfitRm("cores")

         racers[i]:outfitAdd("Unicorp PT-16 Core System")
         racers[i]:outfitAdd("Unicorp D-2 Light Plating")
         local en_choices = {
            "Nexus Dart 150 Engine", "Tricon Zephyr Engine" }
         racers[i]:outfitAdd(en_choices[rnd.rnd(1, #en_choices)])
      end
   end
   for i, j in ipairs(racers) do
      j:rename(fmt.f(_("Racer {n}"), {n=i}))
      j:setHilight(true)
      j:setInvincible(true)
      j:setVisible(true)
      j:control()
      j:face(checkpoint[1]:pos(), true)
      j:broadcast(chatter[i])
   end
   player.pilot():control()
   player.pilot():face(checkpoint[1]:pos(), true)
   countdown = 5 -- seconds
   omsg = player.omsgAdd(tostring(countdown), 0, 50)
   counting = true
   counterhook = hook.timer(1.0, "counter")
   hook.board("board")
   hook.jumpin("jumpin")
   hook.land("land")
end

function counter()
   countdown = countdown - 1
   if countdown == 0 then
      player.omsgChange(omsg, _("Go!"), 1000)
      hook.timer(1.0, "stopcount")
      player.pilot():control(false)
      counting = false
      hook.rm(counterhook)
      for i, j in ipairs(racers) do
         j:control()
         j:moveto(checkpoint[target[i]]:pos())
         hook.pilot(j, "land", "racerland")
      end
      hp1 = hook.pilot(racers[1], "idle", "racer1idle")
      hp2 = hook.pilot(racers[2], "idle", "racer2idle")
      hp3 = hook.pilot(racers[3], "idle", "racer3idle")
      player.msg(_("This race is sponsored by Melendez Corporation. Problem-free ships for problem-free voyages!"))
      else
      player.omsgChange(omsg, tostring(countdown), 0)
      counterhook = hook.timer(1.0, "counter")
   end
end

function racer1idle(p)
   player.msg( fmt.f( _("{pltname} just reached checkpoint {n}"), {pltname=p:name(), n=target[1]}) )
   p:broadcast( fmt.f(_("Checkpoint {n} baby!"), {n=target[1]}) )
   target[1] = target[1] + 1
   hook.timer(2.0, "nexttarget1")
end
function nexttarget1()
   if target[1] == 4 then
      racers[1]:land(curplanet)
      hook.rm(hp1)
      else
      racers[1]:moveto(checkpoint[target[1]]:pos())
   end
end

function racer2idle(p)
   player.msg( fmt.f( _("{pltname} just reached checkpoint {n}"), {pltname=p:name(), n=target[2]}) )
   p:broadcast(_("Hooyah"))
   target[2] = target[2] + 1
   hook.timer(2.0, "nexttarget2")
end
function nexttarget2()
   if target[2] == 4 then
      racers[2]:land(curplanet)
      hook.rm(hp2)
      else
      racers[2]:moveto(checkpoint[target[2]]:pos())
   end
end
function racer3idle(p)
   player.msg( fmt.f( _("{pltname} just reached checkpoint {n}"), {pltname=p:name(), n=target[3]}) )
   p:broadcast(_("Next!"))
   target[3] = target[3] + 1
   hook.timer(2.0, "nexttarget3")
end
function nexttarget3()
   if target[3] == 4 then
      racers[3]:land(curplanet)
      hook.rm(hp3)
   else
      racers[3]:moveto(checkpoint[target[3]]:pos())
   end
end
function stopcount()
   player.omsgRm(omsg)
end
function board(ship)
   for i,j in ipairs(checkpoint) do
      if ship == j and target[4] == i then
         player.msg( fmt.f( _("{pltname} just reached checkpoint {n}"), {pltname=player.name(), n=target[4]}) )
         misn.osdActive(i+1)
         target[4] = target[4] + 1
         if target[4] == 4 then
            tk.msg(fmt.f(_("Checkpoint {n} reached"), {n=i}), fmt.f(_("Proceed to land at {pnt}"), {pnt=curplanet}))
            else
            tk.msg(fmt.f(_("Checkpoint {n} reached"), {n=i}), fmt.f(_("Proceed to Checkpoint {n}"), {n=i+1}))
         end
         break
      end
   end
   player.unboard()
end

function jumpin()
   tk.msg(_("You left the race!"), _([["Because you left the race, you have been disqualified."]]))
   misn.finish(false)
end

function racerland(p)
   player.msg(fmt.f(_("{pltname} just landed at {pnt} and finished the race"), {pltname=p:name(), pnt=curplanet}))
end

function land()
   if target[4] == 4 then
      if racers[1]:exists() and racers[2]:exists() and racers[3]:exists() then
         if choice==2 and player.numOutfit("Racing Trophy") <= 0 then
            tk.msg(_("You Won!"), fmt.f(_([[A man in a suit and tie takes you up onto a stage. A large name tag on his jacket says 'Melendez Corporation'. "Congratulations on your win," he says, shaking your hand, "that was a great race. On behalf of Melendez Corporation, I would like to present to you your trophy and prize money of {credits}!" He hands you one of those fake oversized cheques for the audience, and then a credit chip with the actual prize money on it. At least the trophy looks cool.]]), {credits=fmt.credits(credits)}))
            player.outfitAdd("Racing Trophy")
         else
            tk.msg(_("You Won!"), fmt.f(_([[A man in a suit and tie takes you up onto a stage. A large name tag on his jacket says 'Melendez Corporation'. "Congratulations on your win," he says, shaking your hand, "that was a great race. On behalf of Melendez Corporation, I would like to present to you your prize money of {credits}!" He hands you one of those fake oversized cheques for the audience, and then a credit chip with the actual prize money on it.]]), {credits=fmt.credits(credits)}))
         end
         player.pay(credits)
         misn.finish(true)
      else
         tk.msg(_("You failed to win the race."), _([[As you congratulate the winner on a great race, the laid back person comes up to you.
   "That was a lot of fun! If you ever have time, let's race again. Maybe you'll win next time!"]]))
         misn.finish(false)
      end
   else
      tk.msg(_("You left the race!"), _([["Because you left the race, you have been disqualified."]]))
      misn.finish(false)
   end
end
