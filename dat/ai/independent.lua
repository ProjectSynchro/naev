require 'ai.core.core'
require 'ai.core.idle.civilian'
require 'ai.core.misc.distress'
require "numstring"

mem.careful   = false

function create ()

   -- Credits.
   ai.setcredits( rnd.rnd(ai.pilot():ship():price()/500, ai.pilot():ship():price()/200) )

   -- No bribe
   local bribe_msg = {
      _("\"Just leave me alone!\""),
      _("\"What do you want from me!?\""),
      _("\"Get away from me!\"")
   }
   mem.bribe_no = bribe_msg[ rnd.rnd(1,#bribe_msg) ]

   -- Refuel
   mem.refuel = rnd.rnd( 1000, 3000 )
   local pp = player.pilot()
   if pp:exists() then
      mem.refuel_msg = string.format(_("\"I'll supply your ship with fuel for %s.\""),
            creditstring(mem.refuel))
   end

   mem.loiter = 3 -- This is the amount of waypoints the pilot will pass through before leaving the system
   create_post()
end

