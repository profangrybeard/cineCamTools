@echo off
rem Standalone packaging check. Builds the plugin with no host project.
rem Use this before handing a build to students, not for day to day work.

if "%UE_ROOT%"=="" set UE_ROOT=C:\Program Files\Epic Games\UE_5.8
set REPO=C:\SCAD\Projects\cineCamTools
set OUT=%TEMP%\CineCamTools_Package

"%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%REPO%\CineCamTools.uplugin" -Package="%OUT%" -TargetPlatforms=Win64 -Rocket
echo Packaged to %OUT%
