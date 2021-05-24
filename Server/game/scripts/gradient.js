// This script is to be run with gradient.sh

// Pass it arguments:   startcolor, endcolor, length, and everything after that
// is taken as the string to be colored

const Rhost = await import("https://github.com/stevensmedia/deno-rhost/raw/master/rhost.js")

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
	const startcolor = function() {
		if(!Deno.args[0]) {
			Rhost.print('#-1 startcolor required')
			Deno.exit(-1)
		}
		return Deno.args[0]
	}()

	const endcolor = function() {
		if(!Deno.args[1]) {
			Rhost.print('#-1 endcolor required')
			Deno.exit(-1)
		}
		return Deno.args[1]
	}()

	const length = function() {
		const l = parseInt(Deno.args[2])
		if(typeof l != 'number' || isNaN(l)) {
			Rhost.print('#-1 length required to be a number')
			Deno.exit(-1)
		}
		return l
	}()

	const string = function() {
		if(!Deno.args[3]) {
			Rhost.print('#-1 string required')
			Deno.exit(-1)
		}
		return Deno.args.slice(3).join(' ')
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
		ret += `[ansi(${gradientrange[i % length]},${ch})]`
		++i
	}

	Rhost.print(ret)
}

await main()
