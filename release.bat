@echo off

set RELEASEDIR=%DATE:~0,10%%TIME:~0,5%
set RELEASEDIR=%RELEASEDIR::=%
set RELEASEDIR=%RELEASEDIR:-=%
set RELEASEDIR="Revision\%RELEASEDIR%"
REM echo %RELEASEDIR%
mkdir %RELEASEDIR%\debug
mkdir %RELEASEDIR%\release
copy AndroidMonitor\*.h %RELEASEDIR%\.
copy AndroidMonitor\*.cpp %RELEASEDIR%\.
copy AndroidMonitor\*.rc %RELEASEDIR%\.
copy AndroidMonitor\*.ico %RELEASEDIR%\.
copy AndroidMonitor\ReadMe.txt %RELEASEDIR%\.
copy AndroidMonitor\AndroidMonitor.vcproj %RELEASEDIR%\.
copy debug\*.exe %RELEASEDIR%\debug\.
copy debug\*.dll %RELEASEDIR%\debug\.
copy release\*.exe %RELEASEDIR%\release\.
copy release\*.dll %RELEASEDIR%\release\.
