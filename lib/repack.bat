: # 
: # First portion of file is for a Linux host
: # 
:; ImagePath=$1
:; ImageName=$2
:; PrefixName=$4

:; if [ ! -d temp ]; then 
:; 	mkdir temp 
:; fi

:; cd temp

:; xc32-ar -x ../${ImagePath}

:; for i in `xc32-ar -t ../${ImagePath}`; do
:; 	mv "${i}" "${PrefixName}_${i}"
:; done

:; for i in `xc32-ar -t ../${ImagePath}`; do 
:; 	xc32-ar -r ${ImageName} ${PrefixName}_${i}
:; done

:; cp $2 $3
:; cd ..
:; rm -rf temp




: # 
: # Second  portion of file is for a Windows host
: # 
@echo off
set ImagePath=%1
set ImageName=%2
set PrefixName=%4
if not exist temp (
mkdir temp
)
cd temp
xc32-ar.exe -x ../%ImagePath%
for /f %%i in ('xc32-ar.exe -t ../%ImagePath%') do ren "%%i" "%PrefixName%_%%i"
for /f %%i in ('xc32-ar.exe -t ../%ImagePath%') do xc32-ar.exe -r %ImageName% %PrefixName%_%%i
cp %2 %3
cd ..
del /F /Q temp