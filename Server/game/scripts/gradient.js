// This script is to be run with gradient.sh

// Pass it arguments:   startcolor, endcolor, length, and everything after that
// is taken as the string to be colored

// For example: execscript(gradient.sh,red|blue|20|asdf%tasdf)]

// We rewrite Deno ( https://deno.land/ ) in the path
// and use deno-rhost ( https://github.com/stevensmedia/deno-rhost ) from the net

const Rhost = await import("https://github.com/stevensmedia/deno-rhost/raw/v2/rhost.js")

function getHSV(colorarg) {
	var idx
	try {
		idx = parseInt(colorarg)
		if(isNaN(idx)) {
			throw new Error("NaN!")
		}
	}
	catch(e) {
		idx = Rhost.color.x11ToIndex(colorarg)
	}
	const col = Rhost.color.findXTerm(idx)
	return col.hsv
}

async function main() {
	const rhost = Rhost.environment()

	const startcolor = function() {
		if(!rhost.args[0]) {
			Rhost.print('#-1 startcolor required')
			Deno.exit(-1)
		}
		return rhost.args[0]
	}()

	const endcolor = function() {
		if(!rhost.args[1]) {
			Rhost.print('#-1 endcolor required')
			Deno.exit(-1)
		}
		return rhost.args[1]
	}()

	const length = function() {
		const l = parseInt(rhost.args[2])
		if(typeof l != 'number' || isNaN(l)) {
			Rhost.print('#-1 length required to be a number')
			Deno.exit(-1)
		}
		return l
	}()

	const string = function() {
		if(!rhost.args[3]) {
			Rhost.print('#-1 string required')
			Deno.exit(-1)
		}
		return rhost.args.slice(3).join(' ')
	}()

	const starthsv = function() {
		try {
			return getHSV(startcolor)
		} catch(e) {
			Rhost.print('#-1 startcolor invalid')
			Deno.exit(-1)
		}
	}()

	const endhsv = function() {
		try {
			return getHSV(endcolor)
		} catch(e) {
			Rhost.print('#-1 endcolor invalid')
			Deno.exit(-1)
		}
	}()

	const startrgb = Rhost.color.hsvtorgb(starthsv)
	const endrgb = Rhost.color.hsvtorgb(endhsv)

	const rdiff = endrgb.r - startrgb.r
	const gdiff = endrgb.g - startrgb.g
	const bdiff = endrgb.b - startrgb.b

	var gradientrange = []
	for(var i = 0; i < length; ++i) {
		var nextRGB = {
			r: startrgb.r + (rdiff / (length - 1)) * i,
			g: startrgb.g + (gdiff / (length - 1)) * i,
			b: startrgb.b + (bdiff / (length - 1)) * i
		}
		const rgbHSV = Rhost.color.rgbtohsv(nextRGB)
		gradientrange.push(Rhost.color.hsvToIndex(rgbHSV))
	}

	var ret = ""
	var i = 0
	for(var ch of Rhost.mapString(string)) {
		if(ch.match(/\s/)) {
			ret += ch
		} else {
			ret += `[ansi(${gradientrange[i % length]},${ch})]`
			++i
		}
	}

	Rhost.print(ret)
}

await main()
