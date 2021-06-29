local vn = require 'vn'

--[[
-- Helper functions and defines for the Taiomi campaigns
--]]
local taiomi = {
   scavenger = {
      name = _("Scavenger Drone"),
      portrait = nil,
      image = 'gfx/ship/drone/drone_hyena_comm.webp',
      colour = nil,
   },
   wornout = {
      name = _("Worn-out Drone"),
      portrait = nil,
      image = 'gfx/ship/drone/drone_comm.webp',
      colour = nil,
   },
   philosopher = {
      name = _("Philosopher Drone"),
      portrait = nil,
      image = 'gfx/ship/drone/drone_comm.webp',
      colour = nil,
   },
   log = {
      main = {
         idstr = "log_taiomi_main",
         logname = _("Taiomi"),
         logtype = _("Taiomi"),
      },
   }
}
local function _merge_tables( p, params )
   params = params or {}
   for k,v in pairs(params) do p[k] = v end
   return p
end

-- Helpers to create main characters
function taiomi.vn_scavenger( params )
   return vn.Character.new( taiomi.scavenger.name,
         _merge_tables( {
            image=taiomi.scavenger.image,
            color=taiomi.scavenger.colour,
         }, params) )
end
function taiomi.vn_wornout( params )
   return vn.Character.new( taiomi.wornout.name,
         _merge_tables( {
            image=taiomi.wornout.image,
            color=taiomi.wornout.colour,
         }, params) )
end
function taiomi.vn_philosopher( params )
   return vn.Character.new( taiomi.philosopher.name,
         _merge_tables( {
            image=taiomi.philosopher.image,
            color=taiomi.philosopher.colour,
         }, params) )
end

return taiomi
