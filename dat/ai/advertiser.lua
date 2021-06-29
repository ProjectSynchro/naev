require "ai/tpl/generic"
require "ai/personality/advertiser"
require "ai/include/distress_behaviour"
require "numstring"


function create ()

   -- Credits.
   ai.setcredits( rnd.int(ai.pilot():ship():price()/500, ai.pilot():ship():price()/200) )

   -- No bribe
   local bribe_msg = {
      _("\"Just leave me alone!\""),
      _("\"What do you want from me!?\""),
      _("\"Get away from me!\"")
   }
   mem.bribe_no = bribe_msg[ rnd.int(1,#bribe_msg) ]

   -- Refuel
   mem.refuel = rnd.rnd( 1000, 3000 )
   p = player.pilot()
   if p:exists() then
      standing = ai.getstanding( p ) or -1
      mem.refuel_msg = string.format(_("\"I'll supply your ship with fuel for %s.\""),
            creditstring(mem.refuel));
   end

   -- Selects an advertiser message
   local msg = {
      _("Fly safe, fly Milspec."),
      _("Reynir's Hot Dogs: enjoy the authentic taste of tradition."),
      _("Everyone is faster than light, but only Tricon engines are faster than thought!"),
      _("Dare excellence! Dare Teracom rockets!"),
      _("Most people are ordinary. For the others, Nexus designed the Shark fighter."),
      _("Never take off without your courage. Never take off without your Vendetta."),
      _("Unicorp: low price and high quality!"),
      _("Life is short, spend it at Minerva Station in the Limbo System!"),
   }

   -- Faction specific messages
   local fpres = system.cur():presences()

   local fem = fpres["Empire"] or 0
   if fem > 1 then
      msg[#msg+1] = _("Do you love your Emperor as much as he loves you?")
      msg[#msg+1] = _("You're quick and dependable? The Emperor needs you in the Armada!")
      msg[#msg+1] = _("Help the police help you: denounce the dissidents.")
   end

   local fdv = fpres["Dvaered"] or 0
   if fdv > 1 then
      msg[#msg+1] = _("Only your hard work will save the Dvaered economy!")
      msg[#msg+1] = _("Together, we will beat the economic crisis!")
      msg[#msg+1] = _("Bet on Totoran and win incredible sums thanks to the Crimson Gauntlet!")
   end

   local fsr = fpres["Soromid"] or 0
   if fsr > 1 then
      msg[#msg+1] = _("Special offer on Crow: buy one IR-eye, and the second comes for free!")
      msg[#msg+1] = _("Looking to modify an entire species? Visit Dr. Hu's Gene Clinic at Point Zero Station!")
   end

   local fzl = fpres["Za'lek"] or 0
   if fzl > 1 then
      -- Note that when running in the main menu background, player.name() might not exist (==nil), so 
      -- we need to add a check for that.
      if player.name() then
         msg[#msg+1] = string.format(_("Dear Prof. %s, your recent work has left a deep impression on us. Due to the advance, novelty, and possible wide application of your innovation, we invite you to contribute other unpublished papers of relevant fields to the Interstellar Pay-to-win Journal for Mathematics and Applications."),player.name())
      end
      msg[#msg+1] = _("Want to solve a large-dimensional stochastic PDE? The LMKSTD method is what you need!")
      msg[#msg+1] = _("Love non-convex minimization? Join Ruadan's Computation Science Lab!")
   end

   local fsi = fpres["Sirius"] or 0
   if fsi > 1 then
      msg[#msg+1] = _("Want a new look? Try Verrill's Ceremonial Robes at Burnan!")
   end

   mem.ad = msg[rnd.int(1,#msg)]

   mem.loiter = 3 -- This is the amount of waypoints the pilot will pass through before leaving the system
   create_post()
end

