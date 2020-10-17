-- timeout in milliseconds for detecting consecutive digits
keyTimeout = 100

function onKeyPressed(code, time)
	print("pressed: " .. code .. ", time: " .. time)

	-- left shift pressed
	if code == 160 then
		timeout(100)
	end
	return false
end

function onTimeout()
	if level:len() > 0 then
		c = level:sub(1,1)
		if c == "u" then
			sendKey(38)
		end
		if c == "d" then
			sendKey(40)
		end
		if c == "l" then
			sendKey(37)
		end
		if c == "r" then
			sendKey(39)
		end
		if c == "s" then
			sendKey(32)
		end
		level = level:sub(2)
		timeout(1000)
	else
		exit()
	end
end

level = "rrrrulddluruurrdrdrullurdrddrdlddrrudlluuruluululdrullldddldlldlu"

print("script started")

sendKey(38)
