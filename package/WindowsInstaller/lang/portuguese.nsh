/*
FreeCAD Installer Language File
Language: Portuguese
*/

!insertmacro LANGFILE_EXT "Portuguese"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Instalado para o Utilizador Atual)"

${LangFileString} TEXT_WELCOME "Este assistente de instalação irá guiá-lo através da instalação do $(^NameDA).$\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Visite freecad.org para as últimas notícias, suporte e dicas"

${LangFileString} SecFileAssocTitle "Associação dos ficheiros"
${LangFileString} SecDesktopTitle "Icone do ambiente de trabalho"

${LangFileString} SecCoreDescription "Os ficheiros FreeCAD."
${LangFileString} SecFileAssocDescription "Os ficheiros com a extensão .FCStd irão abrir automaticamente no FreeCAD."
${LangFileString} SecDesktopDescription "Um icone do FreeCAD no ambiente de trabalho."

${LangFileString} AlreadyInstalled "O FreeCAD ${APP_SERIES_KEY2} já está instalado!$\r$\n\
				Quer continuar na mesma a instalar o FreeCAD sobre a versão existente?"
${LangFileString} NewerInstalled "Está a tentar instalar uma versão mais antiga do que a que tem instalada.$\r$\n\
				  Se realmente quer fazer isto deve antes desinstalar o FreeCAD $OldVersionNumber."

${LangFileString} FinishPageRun "Lançar o FreeCAD"

${LangFileString} UnNotInRegistryLabel "Incapaz de encontrar o FreeCAD no registry.$\r$\n\
					Os atalhos para o ambiente de trabalho no menu Start não serão removidos."
${LangFileString} UnInstallRunning "Deve fechar o FreeCAD em primeiro lugar!"
${LangFileString} UnNotAdminLabel "Precisa de privilégios de administrador para desinstalar o FreeCAD!"
${LangFileString} UnReallyRemoveLabel "Tem a certeza que quer remover completamente o FreeCAD e todas as suas componentes?"
${LangFileString} UnFreeCADPreferencesTitle 'Preferências de utilizador do FreeCAD'

${LangFileString} SecUnPreferencesDescription 'Apaga as pastas de configuração do  FreeCAD$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						de todos os utilizadores.'
${LangFileString} SecUnProgramFilesDescription "Desinstala FreeCAD e todas as suas componentes."
