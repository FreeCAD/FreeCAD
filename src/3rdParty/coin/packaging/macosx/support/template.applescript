on run -- for testing in script editor
	process_disk_image("Adium X 1.0b20", "/Users/evands/adium-1.0/Release/Artwork")
end run

on process_disk_image(volumeName)
	tell application "Finder"
		tell disk (volumeName as string)
			open
			
			set theXOrigin to WINX
			set theYOrigin to WINY
			set theWidth to WINW
			set theHeight to WINH
			
			set theBottomRightX to (theXOrigin + theWidth)
			set theBottomRightY to (theYOrigin + theHeight)
			set dsStore to "\"" & "/Volumes/" & volumeName & "/" & ".DS_STORE\""
			--			do shell script "rm " & dsStore
			
			tell container window
				set current view to icon view
				set toolbar visible to false
				set statusbar visible to false
				set the bounds to {theXOrigin, theYOrigin, theBottomRightX, theBottomRightY}
				set statusbar visible to false
			end tell
			
			set opts to the icon view options of container window
			tell opts
				set icon size to ICON_SIZE
				set arrangement to not arranged
			end tell
			-- set background picture of opts to file ".background:background.png"
			BACKGROUND_CLAUSE
			
			-- Positioning
			POSITION_CLAUSE
			-- set position of item "Adium.app" to {196, 273}
			
			-- Custom icons
			-- my copyIconOfTo(artPath & "/ApplicationsIcon", "/Volumes/" & volumeName & "/Applications")
			
			-- Label colors
			-- set label index of item "Adium.app" to 6
			-- set label index of item "License.txt" to 7
			-- set label index of item "Changes.txt" to 7
			-- set label index of item "Applications" to 4
			
			update without registering applications
			-- Force saving of the size
      delay 1
      
			tell container window
				set statusbar visible to false
				set the bounds to {theXOrigin, theYOrigin, theBottomRightX - 10, theBottomRightY - 10}
			end tell
			
			update without registering applications
		end tell
		
    delay 1
		
		tell disk (volumeName as string)
			tell container window
				set statusbar visible to false
				set the bounds to {theXOrigin, theYOrigin, theBottomRightX, theBottomRightY}
			end tell
			
			update without registering applications
	  end tell
		
		--give the finder some time to write the .DS_Store file
    delay 3
    
		set waitTime to 0
		set ejectMe to false
		repeat while ejectMe is false
			delay 1
			set waitTime to waitTime + 1
			
			if (do shell script "[ -f " & dsStore & " ]; echo $?") = "0" then set ejectMe to true
		end repeat
		log "waited " & waitTime & " seconds for .DS_STORE to be created."
	end tell
end process_disk_image

on copyIconOfTo(aFileOrFolderWithIcon, aFileOrFolder)
	tell application "Finder" to set f to POSIX file aFileOrFolderWithIcon as alias
	-- grab the file's icon
	my CopyOrPaste(f, "c")
	-- now the icon is in the clipboard
	tell application "Finder" to set c to POSIX file aFileOrFolder as alias
	my CopyOrPaste(result, "v")
end copyIconOfTo

on CopyOrPaste(i, cv)
	tell application "Finder"
		activate
		open information window of i
	end tell
	tell application "System Events" to tell process "Finder" to tell window 1
		keystroke tab -- select icon button
		keystroke (cv & "w") using command down (* (copy or paste) + close window *)
	end tell -- window 1 then process Finder then System Events
end CopyOrPaste