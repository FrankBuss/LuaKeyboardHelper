-- timeout in milliseconds for detecting consecutive digits
keyTimeout = 100

-- number of digits to detect
detectionLength = 56

-- start index for extraction
startIndex = 18

-- end index for extraction
endIndex = 40

number = ""
lastDigit = ""

function onKeyPressed(code, time)
	-- print("pressed: " .. code .. ", time: " .. time)
	if code >= 48 and code <= 57 then
		lastDigit = code
		number = number .. string.char(code)
		timeout(keyTimeout)
		return true
	end
	return false
end

function onKeyReleased(code, time)
	-- print("released: " .. code .. ", time: " .. time)
	if code >= 48 and code <= 57 then
		print("number: " .. number)
		return true
	end
	return false
end

function onTimeout()
	-- test if 56 digits were received without timeout
	if #number == 56 then
		-- extract digits
		number = string.sub(number, startIndex, endIndex)
	end
	
	-- send extracted digits, or all if interrupted early by a timeout
	for i = 1, #number do
 		sendKey(number:byte(i))
	end
	
	-- reset buffer
	number = ""
end

-- print("script started")
