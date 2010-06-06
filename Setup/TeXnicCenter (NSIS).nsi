; Script generated by the HM NIS Edit Script Wizard.
SetCompressor /SOLID lzma

;!define ADD_VC_RUNTIME

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "TeXnicCenter"
!define PRODUCT_VERSION "2"
!define PRODUCT_PUBLISHER "ToolsCenter"
!define PRODUCT_WEB_SITE "http://www.texniccenter.org"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\TeXnicCenter.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"
!define PRODUCT_SOFTWARE_KEY "Software\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}"

; MUI 1.67 compatible ------
!include "MUI.nsh"
!include "Library.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

!define MUI_COMPONENTSPAGE_SMALLDESC


; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!define MUI_LICENSEPAGE_RADIOBUTTONS
!insertmacro MUI_PAGE_LICENSE "GPL.rtf"
; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Start menu page
var ICONS_GROUP
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "TeXnicCenter"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\TeXnicCenter.exe"
!define MAIN_APPLICATION MUI_FINISHPAGE_RUN

!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "German"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
!insertmacro MUI_RESERVEFILE_LANGDLL

!define VERSION "2.0.0.0"

VIProductVersion "${VERSION}"

VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${PRODUCT_NAME}"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "A test comment"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "TeXnicCenter.org"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "Free software licensed under GPL"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright � 1999-2009 The TeXnicCenter Team"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${PRODUCT_NAME} Setup"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION}"

;VIAddVersionKey /LANG=${LANG_GERMAN} "ProductName" "${PRODUCT_NAME}"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "A test comment"
;VIAddVersionKey /LANG=${LANG_GERMAN} "CompanyName" "ToolsCenter"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "Test Application is a trademark of Fake company"
;VIAddVersionKey /LANG=${LANG_GERMAN} "LegalCopyright" "Copyright � 2005. Alle Rechte vorbehalten."
;VIAddVersionKey /LANG=${LANG_GERMAN} "FileDescription" "${PRODUCT_NAME} Setup"
;VIAddVersionKey /LANG=${LANG_GERMAN} "FileVersion" "${PRODUCT_VERSION}"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "TeXnicCenter Setup.exe"
InstallDir "$PROGRAMFILES\TeXnicCenter"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
;ShowInstDetails show
;ShowUnInstDetails show

RequestExecutionLevel admin

Function .onInit

  ReadRegStr $0 HKLM "Software\Wiegand\TeXnicCenter" "CurrentVersion"
  IfErrors done
  
  MessageBox MB_ICONINFORMATION|MB_YESNO "Setup has found an older version of ${PRODUCT_NAME} on your system.$\n$\nIt is recommended to uninstall older versions, before installing ${PRODUCT_NAME} ${PRODUCT_VERSION}.$\n$\nWould you like to cancel setup now to uninstall the older version of TeXnicCenter?" IDYES quit
  goto done
  
quit:
  Abort
  
done:
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

; English section descriptions
LangString desc_inst_typical ${LANG_ENGLISH} "Typical (Recommended)"
LangString desc_inst_compact ${LANG_ENGLISH} "Compact (Requires less space)"

LangString documentation_folder ${LANG_ENGLISH} "Documentation"
LangString documentation_folder ${LANG_GERMAN} "Dokumentation"

LangString desc_inst_typical ${LANG_GERMAN} "Standard"
LangString desc_inst_compact ${LANG_GERMAN} "Minimal"

InstType "$(desc_inst_typical)"
InstType "$(desc_inst_compact)"

; English section descriptions

LangString desc_core ${LANG_ENGLISH} "${PRODUCT_NAME} core components"
LangString desc_rt ${LANG_ENGLISH} "Runtime libraries"
LangString desc_help ${LANG_ENGLISH} "Help for TeXnicCenter and LaTeX reference"
LangString desc_templates ${LANG_ENGLISH} "Templates for creating new projects and documents"

LangString desc_app_files ${LANG_ENGLISH} "Application Files"
LangString desc_help_files ${LANG_ENGLISH} "Help Files"
LangString desc_latex_templ ${LANG_ENGLISH} "LaTeX Templates"

LangString texniccenter_help ${LANG_ENGLISH} "TeXnicCenter Help"
LangString texniccenter_link ${LANG_ENGLISH} "TeXnicCenter: The (La)TeX IDE"

LangString mbox_uninstall_success ${LANG_ENGLISH} "$(^Name) has been successfully uninstalled."
LangString mbox_uninstall_question ${LANG_ENGLISH} "Do you want to uninstall $(^Name) and all its components?"

LangString sec_dictionaries ${LANG_ENGLISH} "Dictionaries"
LangString sec_create_desktop_link ${LANG_ENGLISH} "Create a link on the desktop"


; German section descriptions

LangString desc_core ${LANG_GERMAN} "${PRODUCT_NAME} Hauptkomponente"
LangString desc_rt ${LANG_GERMAN} "Laufzeit-Bibliotheken"
LangString desc_help ${LANG_GERMAN} "TeXnicCenter Hilfe und LaTeX Referenz"
LangString desc_templates ${LANG_GERMAN} "Projekt- und Dokument-Vorlagen"

LangString desc_app_files ${LANG_GERMAN} "Programmdateien"
LangString desc_help_files ${LANG_GERMAN} "Hilfe-Dateien"
LangString desc_latex_templ ${LANG_GERMAN} "LaTeX Vorlagen"

LangString texniccenter_help ${LANG_GERMAN} "TeXnicCenter Hilfe"
LangString texniccenter_link ${LANG_GERMAN} "TeXnicCenter, die (La)TeX Entwicklungsumgebung"

LangString mbox_uninstall_success ${LANG_GERMAN} "$(^Name) wurde erfolgreich deinstalliert."
LangString mbox_uninstall_question ${LANG_GERMAN} "M�chten Sie $(^Name) und alle seinen Komponenten deinstallieren?"

LangString sec_dictionaries ${LANG_GERMAN} "W�rterb�cher"
LangString sec_create_desktop_link ${LANG_GERMAN} "Desktop Verkn�pfung erstellen"

var "ALREADY_INSTALLED"

Section "$(desc_app_files)" sec_core
SectionIn 1 2 3 RO
  SetOutPath "$SYSDIR"
  SetOverwrite off

  File "msxml4r.dll"
  File "msxml4a.dll"

  StrCpy $ALREADY_INSTALLED 0

  IfFileExists "$SYSDIR\msxml4.dll" 0 install_xml
    StrCpy $ALREADY_INSTALLED 1
    
  goto skip
    
install_xml:
  !insertmacro InstallLib REGDLL $ALREADY_INSTALLED REBOOT_NOTPROTECTED "msxml4.dll" "$SYSDIR\msxml4.dll" "$SYSDIR"
  
  IfErrors next
  goto skip
  
next:
  MessageBox MB_ICONEXCLAMATION|MB_ABORTRETRYIGNORE "Failed to register the MSXML component. Do you want to continue?" IDRETRY install_xml IDABORT abort
  
  goto skip

abort:
  Abort
  
skip:
  
  SetOverwrite on
  
  SetOutPath "$INSTDIR\Language"
  File /r "..\Output\Product\Win32\Unicode Release\Language\*.dll"


  SetOutPath "$INSTDIR"
  File /r "..\Output\Product\\Win32\Unicode Release\Packages"
  
  File "..\Output\Product\Win32\Unicode Release\OutputProfiles.xsd"
  File "..\Output\Product\Win32\Unicode Release\LaTeXCommands.xsd"
  File "..\Output\Product\Win32\Unicode Release\CommonTypes.xsd"
  File "..\Output\Product\Win32\Unicode Release\TeXnicCenterObjects.tlb"
  File "..\Output\Product\Win32\Unicode Release\Tips_german.txt"
  File "..\Output\Product\Win32\Unicode Release\Tips_english.txt"
  File "..\Output\Product\Win32\Unicode Release\msxmlcpp.dll"
  File "..\Output\Product\Win32\Unicode Release\mfcext.dll"
  File "..\Output\Product\Win32\Unicode Release\SciLexer.dll"
  File "..\Output\Product\Win32\Unicode Release\UserImages.bmp"
  File "..\Output\Product\Win32\Unicode Release\TeXnicCenter.exe"

; Shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  
  SetShellVarContext all
  
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\TeXnicCenter.lnk" "$INSTDIR\TeXnicCenter.exe" "" "" "" "" "" "$(texniccenter_link)"
  
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

!ifdef ADD_VC_RUNTIME
Section "Microsoft Visual C++ 2008 Redistributable Package (x86)" sec_rt
SectionIn 1 2 3 RO
  SetOutPath $TEMP
  SetOverwrite on
  File "vcredist_x86.exe"
  ExecWait 'vcredist_x86.exe /q:a /c:"VCREDI~3.EXE /q:a /c:""msiexec /i \vcredist.msi /qb!"" "' $0 ; 3010 reboot req, 0 success
  Delete "vcredist_x86.exe" ; Not needed anymore
SectionEnd
!endif

Section "$(desc_help_files)" sec_help
SectionIn 1 3
  File /r "..\Output\Product\Win32\Unicode Release\Help"
  
  SetShellVarContext all
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\TeXnicCenter Help.lnk" "$INSTDIR\Help\TeXnicCenter.chm" "" "" "" "" "" "$(texniccenter_help)"
SectionEnd

Section "$(desc_latex_templ)" sec_templates
SectionIn 1 3
  File /r "..\Output\Product\Win32\Unicode Release\Templates"
  
  WriteRegStr HKLM "${PRODUCT_SOFTWARE_KEY}\Settings\Options\ProjectTemplatePaths" "AppPath" "$INSTDIR"

  WriteRegStr HKLM "${PRODUCT_SOFTWARE_KEY}\Templates" "DefaultProjectTemplatePath" "$INSTDIR\Templates\Projects"
  WriteRegDword HKCU "${PRODUCT_SOFTWARE_KEY}\Settings\Options\ProjectTemplatePaths" "Size" 1
  WriteRegStr HKCU "${PRODUCT_SOFTWARE_KEY}\Settings\Options\ProjectTemplatePaths" "String0" "$INSTDIR\Templates\Projects"

  WriteRegStr HKLM "${PRODUCT_SOFTWARE_KEY}\Templates" "DefaultDocumentTemplatePath" "$INSTDIR\Templates\Documents"
  WriteRegDword HKCU "${PRODUCT_SOFTWARE_KEY}\Settings\Options\DocumentTemplatePaths" "Size" 1
  WriteRegStr HKCU "${PRODUCT_SOFTWARE_KEY}\Settings\Options\DocumentTemplatePaths" "String0" "$INSTDIR\Templates\Documents"
SectionEnd

SectionGroup "$(sec_dictionaries)"
!define PATH_LANG "$INSTDIR\Dictionaries"

Section "Deutsch"
SectionIn 1 3
SetOutPath ${PATH_LANG}
File /r "..\Dictionaries\*de_*.aff"
File /r "..\Dictionaries\*de_*.dic"
SectionEnd

Section "English"
SectionIn 1 3
SetOutPath ${PATH_LANG}
File /r "..\Dictionaries\*en_*.aff"
File /r "..\Dictionaries\*en_*.dic"
SectionEnd

Section "Fran�ais"
SectionIn 1 3
SetOutPath ${PATH_LANG}
File /r "..\Dictionaries\*fr_FR*.aff"
File /r "..\Dictionaries\*fr_FR*.dic"
SectionEnd

SectionGroupEnd

Section "$(sec_create_desktop_link)" sec_desktop_link
SectionIn 1 3
  SetShellVarContext all
  CreateShortCut "$DESKTOP\TeXnicCenter.lnk" "$INSTDIR\TeXnicCenter.exe" "" "" "" "" "" "$(texniccenter_link)"
SectionEnd

Section "Add TeXnicCenter to the 'Send To' menu" sec_send_to_link
SectionIn 1 3
  SetShellVarContext all
  CreateShortCut "$SENDTO\TeXnicCenter.lnk" "$INSTDIR\TeXnicCenter.exe"
SectionEnd

Section -AdditionalIcons
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  
   SetShellVarContext all
   
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\uninst.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\TeXnicCenter.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\TeXnicCenter.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  
  WriteRegStr HKLM "${PRODUCT_SOFTWARE_KEY}" "CurrentVersion" "${PRODUCT_VERSION}"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${sec_core} $(desc_core)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec_rt} $(desc_rt)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec_templates} $(desc_templates)
  !insertmacro MUI_DESCRIPTION_TEXT ${sec_help} $(desc_help)
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) wurde erfolgreich deinstalliert."
FunctionEnd

Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "$(mbox_uninstall_question)" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP

  RMDir /r "$INSTDIR\Help"
  RMDir /r "$INSTDIR\Templates"
  RMDir /r "$INSTDIR\Language"
  RMDir /r "$INSTDIR\Packages"
  
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\TeXnicCenter.exe"
  Delete "$INSTDIR\UserImages.bmp"
  Delete "$INSTDIR\SciLexer.dll"
  Delete "$INSTDIR\mfcext.dll"
  Delete "$INSTDIR\msxmlcpp.dll"
  Delete "$INSTDIR\Tips_english.txt"
  Delete "$INSTDIR\Tips_german.txt"
  Delete "$INSTDIR\TeXnicCenterObjects.tlb"
  Delete "$INSTDIR\CommonTypes.xsd"
  Delete "$INSTDIR\LaTeXCommands.xsd"
  Delete "$INSTDIR\OutputProfiles.xsd"
  
  RMDir "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  
  DeleteRegKey HKCU "${PRODUCT_SOFTWARE_KEY}"
  DeleteRegKey HKLM "${PRODUCT_SOFTWARE_KEY}"
  
  SetShellVarContext all

  Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\Website.lnk"
  Delete "$DESKTOP\TeXnicCenter.lnk"
  Delete "$SMPROGRAMS\$ICONS_GROUP\TeXnicCenter.lnk"

  RMDir "$SMPROGRAMS\$ICONS_GROUP"
  
  SetAutoClose true
  
SectionEnd
