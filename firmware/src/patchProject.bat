@echo off
setlocal

if exist ..\src\system_config\chipkit_wifire\system_config.h (
call :FindReplace "#define TCPIP_STACK_SECURE_PORT_ENTRIES             10" "#define TCPIP_STACK_SECURE_PORT_ENTRIES             0" ..\src\system_config\chipkit_wifire\system_config.h
)

if exist ..\src\system_config\chipkit_wifire_ec\system_config.h  (
call :FindReplace "#define TCPIP_STACK_SECURE_PORT_ENTRIES             10" "#define TCPIP_STACK_SECURE_PORT_ENTRIES             0" ..\src\system_config\chipkit_wifire_ec\system_config.h
)

exit /b 

:FindReplace <findstr> <replstr> <file>
set tmp="%temp%\tmp.txt"
If not exist %temp%\_.vbs call :MakeReplace
for /f "tokens=*" %%a in ('dir "%3" /s /b /a-d /on') do (
  for /f "usebackq" %%b in (`Findstr /mic:"%~1" "%%a"`) do (
    echo(&Echo Replacing "%~1" with "%~2" in file %%~nxa
    <%%a cscript //nologo %temp%\_.vbs "%~1" "%~2">%tmp%
    if exist %tmp% move /Y %tmp% "%%~dpnxa">nul
  )
)
del %temp%\_.vbs
exit /b

:MakeReplace
>%temp%\_.vbs echo with Wscript
>>%temp%\_.vbs echo set args=.arguments
>>%temp%\_.vbs echo .StdOut.Write _
>>%temp%\_.vbs echo Replace(.StdIn.ReadAll,args(0),args(1),1,-1,1)
>>%temp%\_.vbs echo end with