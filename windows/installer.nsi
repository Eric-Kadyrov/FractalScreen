Unicode true
!ifndef EDITION
  !define EDITION "Free"
!endif
Name "FRACTALSCREEN ${EDITION}"
Caption "FRACTALSCREEN ${EDITION} setup"
OutFile "FractalScreen-${EDITION}-Setup.exe"
RequestExecutionLevel user
InstallDir "$LOCALAPPDATA\FractalScreen"
SetCompressor /SOLID lzma
BrandingText "FRACTALSCREEN"
VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName" "FRACTALSCREEN ${EDITION}"
VIAddVersionKey "FileDescription" "FRACTALSCREEN ${EDITION} setup"
VIAddVersionKey "FileVersion" "1.0.0"
VIAddVersionKey "ProductVersion" "1.0.0"
VIAddVersionKey "CompanyName" "FRACTALSCREEN"
VIAddVersionKey "LegalCopyright" "FRACTALSCREEN"

!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"
!define MUI_ICON "fractalscreen.ico"
!define MUI_UNICON "fractalscreen.ico"
!define MUI_FINISHPAGE_TITLE "FRACTALSCREEN is your screensaver now"
!if "${EDITION}" == "Premium"
!define MUI_FINISHPAGE_TEXT "It will start automatically when your computer has been idle for a few minutes. It keeps running for as long as your subscription is active."
!else
!define MUI_FINISHPAGE_TEXT "It will start automatically when your computer has been idle for a few minutes. Your free trial runs for 2 weeks."
!endif
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Preview it now"
!define MUI_FINISHPAGE_RUN_FUNCTION PreviewNow
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Open screen saver settings"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION OpenSettings
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!if "${EDITION}" == "Premium"
Page custom LicensePage LicensePageLeave
!endif
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

!define UNKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\FractalScreen"

Var LicenseKey
Var KeyField

!if "${EDITION}" == "Premium"
Function LicensePage
  !insertmacro MUI_HEADER_TEXT "Your license key" "Paste the key from your FRACTALSCREEN download page."
  ; Read the clipboard: the download page copies the key when the download button is clicked
  StrCpy $3 ""
  System::Call 'user32::OpenClipboard(p 0)i.r0'
  ${If} $0 <> 0
    System::Call 'user32::GetClipboardData(i 13)p.r1'
    ${If} $1 P<> 0
      System::Call 'kernel32::GlobalLock(p r1)p.r2'
      ${If} $2 P<> 0
        System::Call 'kernel32::lstrcpynW(w .r3, p r2, i 100)'
        System::Call 'kernel32::GlobalUnlock(p r1)'
      ${EndIf}
    ${EndIf}
    System::Call 'user32::CloseClipboard()'
  ${EndIf}
  StrCpy $4 $3 4
  ${If} $4 != "sub_"
    ReadRegStr $3 HKCU "Software\FractalScreen" "LicenseKey"
  ${EndIf}
  nsDialogs::Create 1018
  Pop $0
  ${NSD_CreateLabel} 0 0 100% 36u "Your key starts with sub_. It is shown on the page you downloaded this installer from, and we copied it for you, so it may already be filled in below."
  Pop $0
  ${NSD_CreateText} 0 44u 100% 14u "$3"
  Pop $KeyField
  ${NSD_CreateLabel} 0 66u 100% 24u "No key handy? Leave it empty and enter it later in FRACTALSCREEN settings (Start menu)."
  Pop $0
  nsDialogs::Show
FunctionEnd
Function LicensePageLeave
  ${NSD_GetText} $KeyField $LicenseKey
FunctionEnd
!endif

Section "Install"
  SetOutPath "$INSTDIR"
  File "${EDITION}\FractalScreen.scr"
  ; Same program as an .exe, so desktop and Start menu icons open the app window
  File "/oname=FRACTALSCREEN.exe" "${EDITION}\FractalScreen.scr"
  File "fractalscreen.ico"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
!if "${EDITION}" == "Premium"
  ${If} $LicenseKey != ""
    WriteRegStr HKCU "Software\FractalScreen" "LicenseKey" "$LicenseKey"
    DeleteRegValue HKCU "Software\FractalScreen" "LicenseUntil"
  ${EndIf}
!else
  ; Free trial: 14 days from the first install (kept even if uninstalled)
  ReadRegDWORD $5 HKCU "Software\FractalScreenTrial" "Start"
  ${If} $5 == ""
    System::Call 'msvcrt::time(p 0)i.r5'
    WriteRegDWORD HKCU "Software\FractalScreenTrial" "Start" $5
  ${EndIf}
!endif

  ; Make FRACTALSCREEN the active screensaver for this user
  GetFullPathName /SHORT $0 "$INSTDIR\FractalScreen.scr"
  StrCmp $0 "" 0 +2
    StrCpy $0 "$INSTDIR\FractalScreen.scr"
  WriteRegStr HKCU "Control Panel\Desktop" "SCRNSAVE.EXE" "$0"
  WriteRegStr HKCU "Control Panel\Desktop" "ScreenSaveActive" "1"
  ReadRegStr $1 HKCU "Control Panel\Desktop" "ScreenSaveTimeOut"
  StrCmp $1 "" 0 +2
    WriteRegStr HKCU "Control Panel\Desktop" "ScreenSaveTimeOut" "300"
  ; Tell Windows the setting changed (SPI_SETSCREENSAVEACTIVE, update + broadcast)
  System::Call 'user32::SystemParametersInfoW(i 17, i 1, p 0, i 3)'

  CreateDirectory "$SMPROGRAMS\FRACTALSCREEN"
  Delete "$SMPROGRAMS\FRACTALSCREEN\*.lnk"
  CreateShortcut "$DESKTOP\FRACTALSCREEN.lnk" "$INSTDIR\FRACTALSCREEN.exe" "" "$INSTDIR\fractalscreen.ico" 0 SW_SHOWNORMAL "" "Choose your fractal and play FRACTALSCREEN"
  CreateShortcut "$SMPROGRAMS\FRACTALSCREEN\FRACTALSCREEN.lnk" "$INSTDIR\FRACTALSCREEN.exe" "" "$INSTDIR\fractalscreen.ico" 0 SW_SHOWNORMAL "" "Choose your fractal and play FRACTALSCREEN"
  CreateShortcut "$SMPROGRAMS\FRACTALSCREEN\Play FRACTALSCREEN now.lnk" "$INSTDIR\FRACTALSCREEN.exe" "/s" "$INSTDIR\fractalscreen.ico"
  CreateShortcut "$SMPROGRAMS\FRACTALSCREEN\Uninstall FRACTALSCREEN.lnk" "$INSTDIR\Uninstall.exe"

  WriteRegStr HKCU "${UNKEY}" "DisplayName" "FRACTALSCREEN ${EDITION}"
  WriteRegStr HKCU "${UNKEY}" "DisplayVersion" "1.0.0"
  WriteRegStr HKCU "${UNKEY}" "Publisher" "FRACTALSCREEN"
  WriteRegStr HKCU "${UNKEY}" "DisplayIcon" "$INSTDIR\fractalscreen.ico"
  WriteRegStr HKCU "${UNKEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKCU "${UNKEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNKEY}" "NoRepair" 1
SectionEnd

Function PreviewNow
  Exec '"$INSTDIR\FRACTALSCREEN.exe" /s'
FunctionEnd
Function OpenSettings
  Exec '"$SYSDIR\control.exe" desk.cpl,,@screensaver'
FunctionEnd

Section "Uninstall"
  ReadRegStr $0 HKCU "Control Panel\Desktop" "SCRNSAVE.EXE"
  GetFullPathName /SHORT $1 "$INSTDIR\FractalScreen.scr"
  StrCmp $0 $1 clear 0
  StrCmp $0 "$INSTDIR\FractalScreen.scr" clear done
  clear:
    DeleteRegValue HKCU "Control Panel\Desktop" "SCRNSAVE.EXE"
    System::Call 'user32::SystemParametersInfoW(i 17, i 0, p 0, i 3)'
  done:
  Delete "$INSTDIR\FractalScreen.scr"
  Delete "$INSTDIR\FRACTALSCREEN.exe"
  Delete "$DESKTOP\FRACTALSCREEN.lnk"
  Delete "$INSTDIR\fractalscreen.ico"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\FRACTALSCREEN\*.lnk"
  RMDir "$SMPROGRAMS\FRACTALSCREEN"
  DeleteRegKey HKCU "${UNKEY}"
  DeleteRegKey HKCU "Software\FractalScreen"
SectionEnd
