local function rhost_version()
end

local function rhost_ncomp(x, y)
end

local rhost = {
	version = function()
		return rhost.strfunc("version", "", "")
	end,
	ncomp = function(x, y)
		return rhost.strfunc("ncomp", string.format("%i|%i", x, y), "|")
	end,
}

return rhost
