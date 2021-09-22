--[[
<?xml version='1.0' encoding='utf8'?>
<event name="Derelict">
 <trigger>enter</trigger>
 <chance>30</chance>
 <cond>system.cur():faction() ~= nil</cond>
 <notes>
  <tier>1</tier>
 </notes>
</event>
--]]
--[[
   Derelict Event

   Creates a derelict ship that spawns random events.
--]]
local vntk = require 'vntk'
local fmt = require 'format'

mission_list = {
   --[[
   {
      name = "Mission Name",
      cond = function () return true end, -- Some condition to be met, defaults to true if not set
      weight = 1, -- how it should be weighted, defaults to 1
   },
   --]]
   {
      name = "Derelict Rescue",
   },
}

function create ()
   local nebu_dens, nebu_vol = system.cur():nebula()
   if nebu_vol > 0 then
      evt.finish()
   end

   -- Ignore claimed systems (don't want to ruin the atmosphere)
   if not evt.claim( system.cur(), true ) then evt.finish() end

   -- Get the derelict's ship.
   local r = rnd.rnd()
   if r < 0.2 then
      ship = "Llama"
   elseif r < 0.3 then
      ship = "Hyena"
   elseif r < 0.5 then
      ship = "Koala"
   elseif r < 0.7 then
      ship = "Quicksilver"
   elseif r < 0.9 then
      ship = "Mule"
   else
      ship = "Gawain"
   end

   -- Create the derelict.
   local dist  = rnd.rnd() * system.cur():radius() * 0.6
   local pos   = vec2.newP( dist, rnd.rnd()*360 )
   local p     = pilot.add(ship, "Derelict", pos, nil, {ai="dummy"})
   p:disable()
   p:rename("Derelict")
   p:intrinsicSet( "ew_hide", -75 ) -- Much more visible
   hook.pilot(p, "board", "board")
   hook.pilot(p, "death", "destroyevent")
   hook.jumpout("destroyevent")
   hook.land("destroyevent")
end

function board()
   player.unboard()
   -- Roll for events
   local prob = rnd.rnd()
   if prob <= 0.25 then
      neutralevent()
   elseif prob <= 0.6 then
      goodevent()
   elseif prob <= 0.7 then
      badevent()
   else
      missionevent()
   end
end

function neutralevent()
   local ntitle = _("Empty derelict")
   local ntext = {
      _([[You spend some time searching the derelict, but it doesn't appear there are any remaining passengers, nor is there anything of interest to you. You decide to return to your ship.]]),
      _([[This ship has clearly been abandoned a long time ago. Looters have been here many times, and all of the primary and backup systems are down. However, there is one console that is still operational. It appears to be running an ancient computer game about space exploration, trade and combat. Intrigued, you decide to give the game a try, and before long you find yourself hooked on it. You spend many fun periods hauling cargo, upgrading your ship, fighting enemies and exploring the universe. But of course you can't stay here forever. You regretfully leave the game behind and return to the reality of your everyday life.]]),
      _([[You are exploring the derelict ship when you hear a strange creaking noise. You decide to follow it to see what is causing it, but you never seem to be able to pinpoint the source. After about an hour of fruitlessly opening up panels, pressing your ear against the deck and running hull scans from your own ship, you decide to give up and leave the derelict to its creepy creaking.]]),
      _([[While exploring the cockpit of this derelict, you come across the captain's logs. Hoping to find some information that could be of use to you, you decide to play back the most recent entries. Unfortunately, it turns out the captain's logs are little more than recordings of the captain having heated arguments with her co-pilot. After about ten hectoseconds, you decide you probably aren't going to find anything worthwhile here, so you return to your ship.]]),
      _([[This derelict is not deserted. The crew are still onboard. Unfortunately for them, they didn't survive whatever wrecked their ship. You decide to give them a decent space burial before moving on.]]),
      _([[This derelict seems to have been visited by looters already. You find a message carved into the wall near the airlock. It reads: "I WUS HEAR". Below it is another carved message that says "NO U WASNT". Otherwise, there is nothing of interest left on this ship.]]),
      _([[This derelict seems to have at one time been used as an illegal casino. There are roulette tables and slot machines set up in the cargo hold. However, it seems the local authorities caught wind of the operation, because there are also scorch marks on the furniture and the walls, and there are assault rifle shells underfoot. You don't reckon you're going to find anything here, so you leave.]]),
      _([[When the airlock opens, you are hammered in the face by an ungodly smell that almost makes you pass out on the spot. You hurriedly close the airlock again and flee back into your own ship. Whatever is on that derelict, you don't want to find out!]]),
      _([[This derelict has really been beaten up badly. Most of the corridors are blocked by mangled metal, and your scans read depressurized compartments all over the ship. There's not much you can do here, so you decide to leave the derelict alone.]]),
      _([[The interior of this ship is decorated in a gaudy fashion. There are cute plushies hanging from the doorways, drapes on every viewport, colored pillows in the corners of most compartments and cheerful graffiti on almost all the walls. A scan of the ship's computer shows that this ship belonged to a trio of adventurous young ladies who decided to have a wonderful trip through space. Sadly, it turned out none of them really knew how to fly a space ship, and so they ended up stranded and had to be rescued. Shaking your head, you return to your own ship.]]),
      _([[The artificial gravity on this ship has bizarrely failed, managing to somehow reverse itself. As soon as you step aboard you fall upwards and onto the ceiling, getting some nasty bruises in the process. Annoyed, you search the ship, but without result. You return to your ship - but forget about the polarized gravity at the airlock, so you again smack against the deck plates.]]),
      _([[The cargo hold of this ship contains several heavy, metal chests. You pry them open, but they are empty. Whatever was in them must have been pilfered by other looters already. You decide not to waste any time on this ship, and return to your own.]]),
      _([[You have attached your docking clamp to the derelict's airlock, but the door refuses to open. A few diagnostics reveal that the other side isn't pressurized. The derelict must have suffered hull breaches over the years. It doesn't seem like there's much you can do here.]]),
      _([[As you walk through the corridors of the derelict, you can't help but notice the large scratch marks on the walls, the floor and even the ceiling. It's as if something went on a rampage throughout this ship - something big, with a lot of very sharp claws and teeth… You feel it might be best to leave as soon as possible, so you abandon the search of the derelict and disengage your docking clamp.]]),
   }

   -- Pick a random message from the list, display it, unboard.
   vntk.msg(ntitle, ntext[rnd.rnd(1, #ntext)])
   destroyevent()
end

function goodevent()
   local gtitle = _("Lucky find!")

   local goodevent_list = {
      function ()
         vntk.msg(gtitle, _([[The derelict appears deserted, its passengers long gone. However, they seem to have left behind a small amount of credit chips in their hurry to leave! You decide to help yourself to them, and leave the derelict.]]) )
         player.pay(rnd.rnd(5e3,30e3))
      end,
      function ()
         local factions = {"Empire", "Dvaered", "Sirius", "Soromid", "Za'lek", "Frontier"}
         rndfact = factions[rnd.rnd(1, #factions)]
         vntk.msg(gtitle, fmt.f(_([[This ship looks like any old piece of scrap at a glance, but it is actually an antique, one of the very first of its kind ever produced! Museums all over the galaxy would love to have a ship like this. You plant a beacon on the derelict to mark it for salvaging, and contact the {factname} authorities. Your reputation with them has slightly improved.]]), {factname=rndfact}))
         faction.modPlayerSingle(rndfact, 3)
      end,
   }

   local maps = {
      ["Map: Dvaered-Soromid trade route"] = _("Dvaered-Soromid trade route"),
      ["Map: Sirian border systems"] = _("Sirian border systems"),
      ["Map: Dvaered Core"] = _("Dvaered core systems"),
      ["Map: Empire Core"]  = _("Empire core systems"),
      ["Map: Nebula Edge"]  = _("Sol nebula edge"),
      ["Map: The Frontier"] = _("Frontier systems")
   }
   local unknown = {}
   for k,v in pairs(maps) do
      if player.numOutfit(k) == 0 then
         table.insert( unknown, k )
      end
   end
   -- There are unknown maps
   if #unknown > 0 then
      table.insert( goodevent_list, function ()
         local choice = unknown[rnd.rnd(1,#unknown)]
         vntk.msg(gtitle, fmt.f(_([[The derelict is empty, and seems to have been thoroughly picked over by other space buccaneers. However, the ship's computer contains a map of the {mapname}! You download it into your own computer.]]), {mapname=maps[choice]}))
         player.outfitAdd(choice, 1)
      end )
   end

   -- Run random good event
   goodevent_list[ rnd.rnd(1,#goodevent_list) ]()
   destroyevent()
end


function badevent()
   local btitle = _("Oh no!")
   local badevent_list = {
      function ()
         p:hookClear() -- So the pilot doesn't end the event by dying.
         vntk.msg(btitle, _([[The moment you affix your boarding clamp to the derelict ship, it triggers a booby trap! The derelict explodes, severely damaging your ship. You escaped death this time, but it was a close call!]]))
         p:setHealth(0,0)
         player.pilot():control(true)
         hook.pilot(p, "exploded", "derelict_exploded")
      end,
      function ()
         vntk.msg(btitle, _([[You board the derelict ship and search its interior, but you find nothing. When you return to your ship, however, it turns out there were Space Leeches onboard the derelict - and they've now attached themselves to your ship! You scorch them off with a plasma torch, but it's too late. The little buggers have already drunk all of your fuel. You're not jumping anywhere until you find some more!]]))
         player.pilot():setFuel(false)
         destroyevent()
      end,
      function ()
         vntk.msg(btitle, _([[You affix your boarding clamp and walk aboard the derelict ship. You've only spent a couple of hectoseconds searching the interior when there is a proximity alarm from your ship! Pirates are closing on your position! Clearly this derelict was a trap! You run back onto your ship and prepare to unboard, but you've lost precious time. The pirates are already in firing range…]]))

         local s = player.pilot():ship():size()
         local enemies_tiny = {
            "Hyena",
            "Hyena",
            "Hyena",
         }
         local enemies_sml = {
            "Pirate Vendetta",
            "Pirate Shark",
            "Pirate Shark",
         }
         local enemies med1 = {
            "Pirate Vendetta",
            "Pirate Vendetta",
            "Pirate Ancestor",
            "Pirate Ancestor",
         }
         local enemies med2 = {
            "Pirate Admonisher",
            "Pirate Shark",
            "Pirate Shark",
            "Pirate Shark",
         }
         local enemies_hvy = {
            "Pirate Starbridge",
            "Pirate Vendetta",
            "Pirate Vendetta",
            "Pirate Ancestor",
            "Pirate Ancestor",
         }
         local enemies_dng = {
            "Pirate Kestrel",
            "Pirate Vendetta",
            "Pirate Vendetta",
            "Pirate Ancestor",
            "Pirate Ancestor",
         }
         local enemies
         local r = rnd.rnd()
         if s == 1 then
            enemies = enemies_tiny
         elseif s == 2 then
            if r < 0.5 then
               enemies = enemies_sml
            else
               enemies = enemies_tiny
            end
         elseif s == 3 then
            if r < 0.1 then
               enemies = enemies_med2
            elseif r < 0.6 then
               enemies = enemies_med1
            else
               enemies = enemies_sml
            end
         elseif s == 4 then
            if r < 0.1 then
               enemies = enemies_hvy
            elseif r < 0.6 then
               enemies = enemies_med2
            else
               enemies = enemies_sml
            end
         elseif s == 5 then
            enemies = enemies_hvy
         else
            enemies = enemies_dng
         end
         local pirates = {}
         local pos = player.pos()
         local leader
         for k,v in ipairs(enemies) do
            local dist = 800 + 200 * ship.get(v):size()
            local p = pilot.add( v, "Marauder", pos + vec2.newP( dist + rnd.rnd()*0.5*dist, 360*rnd.rnd() ) )
            if v == "Hyena" then
               p:rename("Pirate Hyena")
            end
            p:setHostile( true ) -- Should naturally attack the player
            table.insert( pirates, p )
            if not leader then
               leader = p
            else
               p:setLeader( leader )
            end
         end
         destroyevent()
      end,
   }

   -- Run random bad event
   badevent_list[ rnd.rnd(1,#badevent_list) ]()
end

function derelict_exploded()
   player.pilot():control(false)
   player.pilot():setHealth(42, 0)
   camera.shake()
   destroyevent()
end

function missionevent()
   -- Fetch all missions that haven't been flagged as done yet.
   local available_missions = {}
   local weights = 0
   for k,m in ipairs(mission_list) do
      if not player.misnDone(m.name) and (not m.cond or m.cond()) then
         weights = weights + (m.weight or 1)
         m.chance = weights
         table.insert( available_missions, m )
      end
   end

   -- If no missions are available, default to a neutral event.
   if #available_missions == 0 then
      neutralevent()
      return
   end

   -- Roll a random mission and start it.
   local r = rnd.rnd()
   for k,m in ipairs(available_missions) do
      if r < m.chance / weights then
         naev.missionStart( m.name )
         destroyevent()
         return
      end
   end
end

function destroyevent()
   evt.finish()
end
