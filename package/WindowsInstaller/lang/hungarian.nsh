/*
Parashell Installer Language File
Language: Hungarian
*/

!insertmacro LANGFILE_EXT "Hungarian"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Telepítve az aktuális felhasználónak)"

${LangFileString} TEXT_WELCOME "A varázsló segítségével tudja telepíteni a Parashell-et.$\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Python parancsfájlok fordítása..."

${LangFileString} TEXT_FINISH_DESKTOP "Indítóikon létrehozása Asztalon"
${LangFileString} TEXT_FINISH_WEBSITE "Látogasson el a parashell.cloud oldalra az aktuális hírekért, támogatásért és tippekért"

#${LangFileString} FileTypeTitle "Parashell-dokumentum"

#${LangFileString} SecAllUsersTitle "Telepítés minden felhasználónak"
${LangFileString} SecFileAssocTitle "Fájltársítások"
${LangFileString} SecDesktopTitle "Parancsikon Asztalra"

${LangFileString} SecCoreDescription "A Parashell futtatásához szükséges fájlok."
#${LangFileString} SecAllUsersDescription "Minden felhasználónak telepítsem vagy csak az aktuálisnak?"
${LangFileString} SecFileAssocDescription "A .FCStd kiterjesztéssel rendelkező fájlok megnyitása automatikusan a Parashell-el történjen."
${LangFileString} SecDesktopDescription "Parashell-ikon elhelyezése az Asztalon."
#${LangFileString} SecDictionaries "Szótárak"
#${LangFileString} SecDictionariesDescription "Helyesírás-ellenőrző szótárak, amiket letölthet és telepíthet."

#${LangFileString} PathName 'A $\"xxx.exe$\" fájl elérési útja'
#${LangFileString} InvalidFolder 'Nem találom a $\"xxx.exe$\" fájlt, a megadott helyen.'

#${LangFileString} DictionariesFailed 'Szótár letöltése a(z) $\"$R3$\" nyelvhez sikertelen.'

#${LangFileString} ConfigInfo "A Parashell telepítés utáni beállítása hosszú időt vehet igénybe."

#${LangFileString} RunConfigureFailed "Nem tudom végrehajtani a configure parancsfájlt!"
${LangFileString} InstallRunning "A telepítő már fut!"
${LangFileString} AlreadyInstalled "A Parashell ${APP_SERIES_KEY2} már teleptve van!$\r$\n\
				Installing over existing installations is not recommended if the installed version$\r$\n\
				is a test release or if you have problems with your existing Parashell installation.$\r$\n\
				In these cases better reinstall Parashell.$\r$\n\
				Dou you nevertheles want to install Parashell over the existing version?"
${LangFileString} NewerInstalled "A jelenleg telepítettnél régebbi Parashell verziót próbál telepíteni.$\r$\n\
				  Ha valóban ezt akarja, először el kell távolítania a meglévő Parashell $OldVersionNumber változatot."

#${LangFileString} FinishPageMessage "Gratulálok! Sikeresen telepítette a Parashell-et.$\r$\n\
#					$\r$\n\
#					(A program első indítása egy kis időt vehet igénybe...)"
${LangFileString} FinishPageRun "Parashell indítása"

${LangFileString} UnNotInRegistryLabel "Nem találom a Parashell-et a regisztriben.$\r$\n\
					Az Asztalon és a Start Menüben található parancsikonok nem lesznek eltávolítva!."
${LangFileString} UnInstallRunning "Először be kell zárnia a Parashell-et!"
${LangFileString} UnNotAdminLabel "A Parashell eltávolításhoz rendszergazdai jogokkal kell rendelkeznie!"
${LangFileString} UnReallyRemoveLabel "Biztosan abban, hogy el akarja távolítani a Parashell-t, minden tartozékával együtt?"
${LangFileString} UnFreeCADPreferencesTitle 'Parashell felhasználói beállítások'

#${LangFileString} SecUnProgDescription "xxx eltávolítása."
${LangFileString} SecUnPreferencesDescription 'A  Parashell beállítások mappa törlése$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						minden felhasználónál.'
${LangFileString} DialogUnPreferences 'You chose to delete the Parashells user configuration.$\r$\n\
						This will also delete all installed Parashell addons.$\r$\n\
						Do you agree with this?'
${LangFileString} SecUnProgramFilesDescription "A Parashell és minden komponensének eltávolítása."
