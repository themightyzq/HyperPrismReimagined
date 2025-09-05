; HyperPrism Reimagined - Inno Setup Script
; Creates Windows installer for VST3 plugins

#define MyAppName "HyperPrism Reimagined"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "HyperPrism"
#define MyAppURL "https://hyperprism.com"

[Setup]
AppId={{E8A3B7C5-4D2F-4A1B-9C3D-2E1F0A9B8C7D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commoncf64}\VST3
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE.txt
InfoBeforeFile=..\..\README.txt
OutputDir=..\..\Distribution\Windows
OutputBaseFilename=HyperPrism_Reimagined_v{#MyAppVersion}_Setup
SetupIconFile=..\..\Resources\icon.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
SelectDirDesc=Select VST3 Plugin Directory
SelectDirLabel3=Setup will install the VST3 plugins into the following folder.
SelectDirBrowseLabel=To continue, click Next. If you would like to select a different folder, click Browse.

[Types]
Name: "full"; Description: "Full installation (all plugins)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "dynamics"; Description: "Dynamics & Compression"; Types: full
Name: "dynamics\compressor"; Description: "Compressor"; Types: full
Name: "dynamics\limiter"; Description: "Limiter"; Types: full
Name: "dynamics\noisegate"; Description: "Noise Gate"; Types: full
Name: "dynamics\stereodynamics"; Description: "Stereo Dynamics"; Types: full

Name: "filters"; Description: "Filters & EQ"; Types: full
Name: "filters\highpass"; Description: "High Pass"; Types: full
Name: "filters\lowpass"; Description: "Low Pass"; Types: full
Name: "filters\bandpass"; Description: "Band Pass"; Types: full
Name: "filters\bandreject"; Description: "Band Reject"; Types: full

Name: "delays"; Description: "Delays & Time Effects"; Types: full
Name: "delays\delay"; Description: "Delay"; Types: full
Name: "delays\singledelay"; Description: "Single Delay"; Types: full
Name: "delays\echo"; Description: "Echo"; Types: full
Name: "delays\multidelay"; Description: "Multi Delay"; Types: full

Name: "modulation"; Description: "Modulation Effects"; Types: full
Name: "modulation\chorus"; Description: "Chorus"; Types: full
Name: "modulation\flanger"; Description: "Flanger"; Types: full
Name: "modulation\phaser"; Description: "Phaser"; Types: full
Name: "modulation\hyperphaser"; Description: "HyperPhaser"; Types: full
Name: "modulation\tremolo"; Description: "Tremolo"; Types: full
Name: "modulation\vibrato"; Description: "Vibrato"; Types: full
Name: "modulation\autopan"; Description: "AutoPan"; Types: full

Name: "pitch"; Description: "Pitch & Frequency"; Types: full
Name: "pitch\pitchchanger"; Description: "Pitch Changer"; Types: full
Name: "pitch\frequencyshifter"; Description: "Frequency Shifter"; Types: full
Name: "pitch\ringmodulator"; Description: "Ring Modulator"; Types: full
Name: "pitch\vocoder"; Description: "Vocoder"; Types: full

Name: "spatial"; Description: "Spatial & Stereo"; Types: full
Name: "spatial\pan"; Description: "Pan"; Types: full
Name: "spatial\quasistereo"; Description: "Quasi Stereo"; Types: full
Name: "spatial\morestereo"; Description: "More Stereo"; Types: full
Name: "spatial\msmatrix"; Description: "MS Matrix"; Types: full
Name: "spatial\reverb"; Description: "Reverb"; Types: full

Name: "distortion"; Description: "Distortion & Enhancement"; Types: full
Name: "distortion\tubetape"; Description: "Tube/Tape Saturation"; Types: full
Name: "distortion\harmonic"; Description: "Harmonic Exciter"; Types: full
Name: "distortion\decimator"; Description: "Sonic Decimator"; Types: full
Name: "distortion\bassmax"; Description: "Bass Maximizer"; Types: full

[Files]
; Dynamics
Source: "..\..\Distribution\Windows\VST3\Compressor.vst3\*"; DestDir: "{app}\HyperPrism Reimagined\Compressor.vst3"; Flags: ignoreversion recursesubdirs; Components: dynamics\compressor
Source: "..\..\Distribution\Windows\VST3\Limiter.vst3\*"; DestDir: "{app}\HyperPrism Reimagined\Limiter.vst3"; Flags: ignoreversion recursesubdirs; Components: dynamics\limiter
Source: "..\..\Distribution\Windows\VST3\NoiseGate.vst3\*"; DestDir: "{app}\HyperPrism Reimagined\NoiseGate.vst3"; Flags: ignoreversion recursesubdirs; Components: dynamics\noisegate
Source: "..\..\Distribution\Windows\VST3\StereoDynamics.vst3\*"; DestDir: "{app}\HyperPrism Reimagined\StereoDynamics.vst3"; Flags: ignoreversion recursesubdirs; Components: dynamics\stereodynamics

; Filters
Source: "..\..\Distribution\Windows\VST3\HighPass.vst3\*"; DestDir: "{app}\HyperPrism Reimagined\HighPass.vst3"; Flags: ignoreversion recursesubdirs; Components: filters\highpass
Source: "..\..\Distribution\Windows\VST3\LowPass.vst3\*"; DestDir: "{app}\HyperPrism Reimagined\LowPass.vst3"; Flags: ignoreversion recursesubdirs; Components: filters\lowpass
Source: "..\..\Distribution\Windows\VST3\BandPass.vst3\*"; DestDir: "{app}\HyperPrism Reimagined\BandPass.vst3"; Flags: ignoreversion recursesubdirs; Components: filters\bandpass
Source: "..\..\Distribution\Windows\VST3\BandReject.vst3\*"; DestDir: "{app}\HyperPrism Reimagined\BandReject.vst3"; Flags: ignoreversion recursesubdirs; Components: filters\bandreject

; Continue for all other plugins...
; (Add all 32 plugins following the same pattern)

[Icons]
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

[Code]
function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Skip the select directory page if installing to default VST3 location
  Result := False;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    // Offer to launch DAW or rescan plugins
    if MsgBox('Installation complete!' + #13#10 + #13#10 + 
              'The plugins have been installed to:' + #13#10 + 
              ExpandConstant('{app}') + #13#10 + #13#10 +
              'You may need to rescan your plugins in your DAW.' + #13#10 + #13#10 +
              'Would you like to open the plugin folder?', 
              mbConfirmation, MB_YESNO) = IDYES then
    begin
      ShellExec('open', ExpandConstant('{app}'), '', '', SW_SHOW, ewNoWait, ResultCode);
    end;
  end;
end;