@echo off
rem Junctions this repo into the workbench project's Plugins folder.
rem Run once. Safe to re-run: it stops if the link already exists.

set REPO=C:\SCAD\Projects\cineCamTools
set WORKBENCH=C:\_projects\pluginWorkbench
set LINK=%WORKBENCH%\Plugins\CineCamTools

if not exist "%WORKBENCH%" (
	echo Workbench not found at %WORKBENCH%
	exit /b 1
)

if exist "%LINK%" (
	echo Already exists: %LINK%
	dir /AL "%WORKBENCH%\Plugins"
	exit /b 0
)

if not exist "%WORKBENCH%\Plugins" mkdir "%WORKBENCH%\Plugins"
mklink /J "%LINK%" "%REPO%"
