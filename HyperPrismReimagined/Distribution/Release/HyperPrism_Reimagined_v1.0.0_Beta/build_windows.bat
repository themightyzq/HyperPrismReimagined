@echo off
REM HyperPrism Reimagined - Windows Build Script
REM Builds VST3 plugins for Windows distribution

setlocal enabledelayedexpansion

echo === HyperPrism Reimagined Windows Build Script ===

REM Configuration
set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\Builds\VisualStudio2022
set OUTPUT_DIR=%PROJECT_ROOT%\Distribution\Windows
set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

REM Create output directory
if not exist "%OUTPUT_DIR%\VST3" mkdir "%OUTPUT_DIR%\VST3"

REM List of all plugins
set PLUGINS=AutoPan BandPass BandReject BassMaximiser Chorus Compressor Delay Echo Flanger FrequencyShifter HarmonicExciter HighPass HyperPhaser Limiter LowPass MoreStereo MSMatrix MultiDelay NoiseGate Pan Phaser PitchChanger QuasiStereo Reverb RingModulator SingleDelay SonicDecimator StereoDynamics Tremolo TubeTapeSaturation Vibrato Vocoder

REM Build each plugin
for %%P in (%PLUGINS%) do (
    echo.
    echo Building %%P...
    
    if exist "%PROJECT_ROOT%\%%P\%%P.jucer" (
        REM Resave with Projucer (adjust path as needed)
        "C:\JUCE\Projucer.exe" --resave "%PROJECT_ROOT%\%%P\%%P.jucer"
        
        REM Build with MSBuild
        if exist "%BUILD_DIR%\%%P.sln" (
            %MSBUILD% "%BUILD_DIR%\%%P.sln" /p:Configuration=Release /p:Platform=x64
            
            REM Copy VST3 to output
            if exist "%BUILD_DIR%\x64\Release\VST3\%%P.vst3" (
                xcopy /E /I /Y "%BUILD_DIR%\x64\Release\VST3\%%P.vst3" "%OUTPUT_DIR%\VST3\%%P.vst3\"
                echo %%P built successfully
            ) else (
                echo Warning: %%P.vst3 not found in build output
            )
        ) else (
            echo Warning: %%P.sln not found
        )
    ) else (
        echo Warning: %%P.jucer not found
    )
)

REM Create ZIP for distribution
echo.
echo Creating distribution ZIP...
cd /d "%OUTPUT_DIR%"
"C:\Program Files\7-Zip\7z.exe" a -tzip "HyperPrism_Reimagined_v1.0_Windows.zip" VST3

echo.
echo === Build Complete ===
echo VST3s are in: %OUTPUT_DIR%\VST3
echo Distribution ZIP: %OUTPUT_DIR%\HyperPrism_Reimagined_v1.0_Windows.zip

pause