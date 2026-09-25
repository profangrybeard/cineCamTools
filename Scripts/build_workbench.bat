@echo off
rem Builds the workbench editor target, which compiles this plugin through the junction.
rem Set UE_ROOT if the engine is not in the default launcher location.

if "%UE_ROOT%"=="" set UE_ROOT=C:\Program Files\Epic Games\UE_5.8
set WORKBENCH=C:\_projects\pluginWorkbench
set UPROJECT=%WORKBENCH%\pluginWorkbench.uproject

if not exist "%UPROJECT%" (
	echo Project not found: %UPROJECT%
	echo Fix UPROJECT in this script if the .uproject has a different name.
	exit /b 1
)

"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" pluginWorkbenchEditor Win64 Development -Project="%UPROJECT%" -WaitMutex -FromMsBuild
