--[[
<?xml version='1.0' encoding='utf8'?>
<mission name="Empire Shipping">
  <avail>
   <priority>3</priority>
   <cond>faction.playerStanding("Empire") &gt;= 0 and var.peek("es_cargo") == true</cond>
   <chance>350</chance>
   <done>Empire Recruitment</done>
   <location>Computer</location>
   <faction>Empire</faction>
  </avail>
 </mission>
 --]]
--[[

   Handles the randomly generated Empire cargo missions.

]]--

require "cargo_common"
require "numstring"


misn_desc  = _("Official Empire cargo transport to %s in the %s system.")

piracyrisk = {}
piracyrisk[1] = _("Piracy Risk: None")
piracyrisk[2] = _("Piracy Risk: Low")
piracyrisk[3] = _("Piracy Risk: Medium")
piracyrisk[4] = _("Piracy Risk: High")

msg_timeup = _("MISSION FAILED: You have failed to deliver the goods to the Empire on time!")

osd_title = _("Empire Shipping")
osd_msg1 = _("Fly to %s in the %s system before %s\n(%s remaining)")

--[[
--    Empire shipping missions are always timed, but quite lax on the schedules
--    pays a bit more then the rush missions
--]]

-- Create the mission
function create()
   -- Note: this mission does not make any system claims.

   origin_p, origin_s = planet.cur()
   local routesys = origin_s
   local routepos = origin_p:pos()

   -- target destination
   destplanet, destsys, numjumps, traveldist, cargo, avgrisk, tier = cargo_calculateRoute()
   if destplanet == nil then
      misn.finish(false)
   end
   if destplanet:faction() ~= faction.get( "Empire" ) then
      misn.finish(false)
   end

   -- mission generics
   stuperpx   = 0.3 - 0.015 * tier
   stuperjump = 11000 - 75 * tier
   stupertakeoff = 15000
   timelimit  = time.get() + time.create(0, 0, traveldist * stuperpx + numjumps * stuperjump + stupertakeoff + 480 * numjumps)

   -- Allow extra time for refuelling stops.
   local jumpsperstop = 3 + math.min(tier, 2)
   if numjumps > jumpsperstop then
      timelimit:add(time.create( 0, 0, math.floor((numjumps-1) / jumpsperstop) * stuperjump ))
   end

   --Determine risk of piracy
    if avgrisk == 0 then
      piracyrisk = piracyrisk[1]
      riskreward = 0
   elseif avgrisk <= 25 then
      piracyrisk = piracyrisk[2]
      riskreward = 10
   elseif avgrisk > 25 and avgrisk <= 100 then
      piracyrisk = piracyrisk[3]
      riskreward = 25
   else
      piracyrisk = piracyrisk[4]
      riskreward = 50
   end

   -- Choose amount of cargo and mission reward. This depends on the mission tier.
   finished_mod = 2.0 -- Modifier that should tend towards 1.0 as Naev is finished as a game
   amount     = rnd.rnd(10 + 3 * tier, 20 + 4 * tier)
   jumpreward = commodity.price(cargo)*1.5
   distreward = math.log(300*commodity.price(cargo))/100
   reward     = 1.5^tier * (avgrisk*riskreward + numjumps * jumpreward + traveldist * distreward) * finished_mod * (1. + 0.05*rnd.twosigma())

   misn.setTitle( string.format(
      _("ES: Cargo transport to %s in %s (%s)"), destplanet:name(), destsys:name(), tonnestring(amount) ) )
   misn.markerAdd(destsys, "computer")
   cargo_setDesc( misn_desc:format( destplanet:name(), destsys:name() ), cargo, amount, destplanet, timelimit, piracyrisk );
   misn.setReward( creditstring(reward) )
end

-- Mission is accepted
function accept()
   local playerbest = cargoGetTransit( timelimit, numjumps, traveldist )
   if timelimit < playerbest then
      if not tk.yesno( _("Too slow"), string.format(
            _("This shipment must arrive within %s, but it will take at least %s for your ship to reach %s, missing the deadline. Accept the mission anyway?"),
            (timelimit - time.get()):str(), (playerbest - time.get()):str(),
            destplanet:name() ) ) then
         misn.finish()
      end
   end
   if player.pilot():cargoFree() < amount then
      tk.msg( _("No room in ship"), string.format(
         _("You don't have enough cargo space to accept this mission. It requires %s of free space (%s more than you have)."),
         tonnestring(amount),
         tonnestring( amount - player.pilot():cargoFree() ) ) )
      misn.finish()
   end

   misn.accept()

   carg_id = misn.cargoAdd( cargo, amount )
   tk.msg( _("Mission Accepted"), string.format(
      _("The Empire workers load the %s of %s onto your ship."),
      tonnestring(amount), _(cargo) ) )
   local osd_msg = {}
   osd_msg[1] = osd_msg1:format(
      destplanet:name(), destsys:name(), timelimit:str(),
      ( timelimit - time.get() ):str() )
   misn.osdCreate(osd_title, osd_msg)
   hook.land( "land" ) -- only hook after accepting
   hook.date(time.create(0, 0, 100), "tick") -- 100STU per tick
end

-- Land hook
function land()
   if planet.cur() == destplanet then
      tk.msg( _("Successful Delivery"), string.format(
         _("The Empire workers unload the %s at the docks."), _(cargo) ) )
      player.pay(reward)
      n = var.peek("es_misn")
      if n ~= nil then
         var.push("es_misn", n+1)
      else
         var.push("es_misn", 1)
      end

      -- increase faction
      faction.modPlayerSingle("Empire", rnd.rnd(2, 4))
      misn.finish(true)
   end
end

-- Date hook
function tick()
   if timelimit >= time.get() then
      -- Case still in time
      local osd_msg = {}
      osd_msg[1] = osd_msg1:format(
         destplanet:name(), destsys:name(), timelimit:str(),
         ( timelimit - time.get() ):str() )
      misn.osdCreate(osd_title, osd_msg)
   elseif timelimit <= time.get() then
      -- Case missed deadline
      player.msg(msg_timeup)
      misn.finish(false)
   end
end
