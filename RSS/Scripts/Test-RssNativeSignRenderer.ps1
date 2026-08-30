<#
.SYNOPSIS
Builds and validates RSS native sign rendering without stopping Unreal Editor or Satisfactory.

.DESCRIPTION
Checks every RSS Unreal Python script for syntax errors and builds RSS Development Editor code when
Unreal Editor is closed. After a successful build, it starts UnrealEditor-Cmd for guarded axis-label
repair and transient-widget behavior validation. With one editor already open, -SkipEditorBuild uses
Python Remote Execution instead.

An open editor locks UnrealEditor-RSS.dll. In that state, use -SkipEditorBuild only after the editor
has loaded the intended RSS DLL. The Unreal stage then fixes the Horizontal/Vertical editor labels
and runs non-destructive transient-widget behavior checks. Both JSON reports must say "passed", and
every reported check must pass.

Packaging, installation, and launch are opt-in. -Install implies -Package. -Launch implies both
-Install and -Package. This script never stops an existing editor or game process.

.PARAMETER SkipEditorBuild
Skips the RSS Development Editor build. Required when validating against an already-running editor.

.PARAMETER SkipUnrealValidation
Skips headless/remote axis-label repair and transient-widget validation.

.PARAMETER Package
Builds Shipping code and packages RSS through the repository runtime-test wrapper.

.PARAMETER Install
Packages RSS and copies it into the configured Satisfactory installation. Fails if game is running.

.PARAMETER Launch
Installs RSS, launches Satisfactory with latest or selected save, and performs stable log assertion.
Requires -LogPattern. Existing game process is never stopped.

.PARAMETER LogPattern
Literal log text expected during -Launch runtime verification.

.PARAMETER ExpectedCount
Exact stable count required for LogPattern during -Launch. Default: 1.

.PARAMETER ProjectRoot
FactoryGame workspace root. Default resolves four levels above this script.

.PARAMETER EngineRoot
Unreal Engine CSS installation root.

.PARAMETER GameRoot
Satisfactory installation root used by -Install and -Launch.

.PARAMETER SteamExe
Steam executable used by -Launch.

.PARAMETER SavePath
Optional save used by -Launch. Runtime wrapper selects latest save when omitted.

.PARAMETER PythonExecutable
Python command or executable path used for syntax checks and Unreal remote execution.

.EXAMPLE
PS> .\Mods\GameFeatures\RSS\Scripts\Test-RssNativeSignRenderer.ps1

Editor closed: syntax-checks, builds RSS Development Editor module, then validates through commandlet.

.EXAMPLE
PS> .\Mods\GameFeatures\RSS\Scripts\Test-RssNativeSignRenderer.ps1 -SkipEditorBuild

Editor open with current DLL: runs guarded axis-label repair and native renderer validation.

.EXAMPLE
PS> .\Mods\GameFeatures\RSS\Scripts\Test-RssNativeSignRenderer.ps1 -SkipEditorBuild -Package

Validates through open editor, then builds and packages RSS Shipping binaries.

.EXAMPLE
PS> .\Mods\GameFeatures\RSS\Scripts\Test-RssNativeSignRenderer.ps1 -SkipEditorBuild -Launch -LogPattern 'RSS native renderer ready' -ExpectedCount 1

Validates editor state, packages, installs, launches, and performs stable runtime log assertion.
#>
[CmdletBinding()]
param(
	[switch]$SkipEditorBuild,
	[switch]$SkipUnrealValidation,
	[switch]$Package,
	[switch]$Install,
	[switch]$Launch,

	[string]$LogPattern,

	[ValidateRange(0, 100000)]
	[int]$ExpectedCount = 1,

	[string]$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\..')),
	[string]$EngineRoot = 'C:\Program Files\Unreal Engine - CSS',
	[string]$GameRoot = 'C:\Program Files (x86)\Steam\steamapps\common\Satisfactory',
	[string]$SteamExe = 'C:\Program Files (x86)\Steam\steam.exe',
	[string]$SavePath,
	[string]$PythonExecutable = 'python'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-NativeExitCode
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Action
	)

	if ($LASTEXITCODE -ne 0)
	{
		throw "$Action failed with exit code $LASTEXITCODE."
	}
}

function Get-RunningUnrealEditor
{
	return @(Get-Process -Name 'UnrealEditor*', 'FactoryEditor*' -ErrorAction SilentlyContinue |
		Where-Object { $_.ProcessName -notlike '*-Cmd' } |
		Sort-Object -Property Id -Unique)
}

function Assert-PythonSyntax
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Python,

		[Parameter(Mandatory = $true)]
		[System.IO.FileInfo]$Script
	)

	$SyntaxProbe = @'
import ast
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
ast.parse(path.read_text(encoding="utf-8-sig"), filename=str(path))
print(f"Python syntax passed: {path}")
'@
	& $Python -c $SyntaxProbe $Script.FullName
	Assert-NativeExitCode -Action "Python syntax check for $($Script.FullName)"
}

function Invoke-RemoteUnrealScript
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Python,

		[Parameter(Mandatory = $true)]
		[string]$RemoteRunner,

		[Parameter(Mandatory = $true)]
		[string]$Script
	)

	$RemoteScriptPath = $Script.Replace('\', '/')
	& $Python $RemoteRunner --script $RemoteScriptPath | Out-Host
	return [int]$LASTEXITCODE
}

function Invoke-HeadlessUnrealScript
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$UnrealEditorCmd,

		[Parameter(Mandatory = $true)]
		[string]$Project,

		[Parameter(Mandatory = $true)]
		[string]$Script
	)

	$HeadlessScriptPath = $Script.Replace('\', '/')
	& $UnrealEditorCmd $Project -run=pythonscript "-script=$HeadlessScriptPath" -unattended -nop4 -nosplash -nullrhi -nosound -stdout -FullStdOutLogOutput | Out-Host
	return [int]$LASTEXITCODE
}

function Read-PassedJsonReport
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path,

		[Parameter(Mandatory = $true)]
		[datetime]$StartedAtUtc,

		[switch]$RequireVerified,

		[switch]$RejectLimitations
	)

	if (-not (Test-Path -LiteralPath $Path -PathType Leaf))
	{
		throw "Unreal validation report missing: $Path"
	}

	$ReportFile = Get-Item -LiteralPath $Path
	if ($ReportFile.LastWriteTimeUtc -lt $StartedAtUtc)
	{
		throw "Unreal validation report is stale: $Path (written $($ReportFile.LastWriteTimeUtc.ToString('o')))."
	}

	try
	{
		$Report = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
	}
	catch
	{
		throw "Invalid JSON report $Path`: $($_.Exception.Message)"
	}

	if (-not ($Report.PSObject.Properties.Name -contains 'status'))
	{
		throw "JSON report has no status: $Path"
	}
	if ([string]$Report.status -ne 'passed')
	{
		$Reasons = @()
		if ($Report.PSObject.Properties.Name -contains 'blocked_reason')
		{
			$Reasons += [string]$Report.blocked_reason
		}
		if ($Report.PSObject.Properties.Name -contains 'errors')
		{
			$Reasons += @($Report.errors | ForEach-Object {
				if ($_.PSObject.Properties.Name -contains 'message') { [string]$_.message } else { [string]$_ }
			})
		}
		if ($Report.PSObject.Properties.Name -contains 'limitations')
		{
			$Reasons += @($Report.limitations | ForEach-Object { "limitation: $([string]$_)" })
		}
		if ($Report.PSObject.Properties.Name -contains 'checks')
		{
			$Reasons += @($Report.checks |
				Where-Object { $_.passed -ne $true } |
				ForEach-Object { "failed check: $([string]$_.name)" })
		}
		$ReasonText = if ($Reasons.Count -gt 0) { $Reasons -join '; ' } else { 'no detail reported' }
		throw "Unreal validation report status is '$($Report.status)': $Path. $ReasonText"
	}
	if ($Report.PSObject.Properties.Name -contains 'errors')
	{
		$ReportedErrors = @($Report.errors | ForEach-Object {
			if ($_.PSObject.Properties.Name -contains 'message') { [string]$_.message } else { [string]$_ }
		} | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
		if ($ReportedErrors.Count -gt 0)
		{
			throw "Passed Unreal report still contains errors in $Path`: $($ReportedErrors -join '; ')"
		}
	}
	if ($RejectLimitations -and $Report.PSObject.Properties.Name -contains 'limitations')
	{
		$Limitations = @($Report.limitations | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
		if ($Limitations.Count -gt 0)
		{
			throw "Unreal behavior validation is incomplete in $Path`: $($Limitations -join '; ')"
		}
	}

	if ($RequireVerified)
	{
		if (-not ($Report.PSObject.Properties.Name -contains 'verified') -or $Report.verified -ne $true)
		{
			throw "Axis-label report did not verify saved labels: $Path"
		}
	}

	if ($Report.PSObject.Properties.Name -contains 'checks')
	{
		$FailedChecks = @($Report.checks | Where-Object { $_.passed -ne $true })
		if ($FailedChecks.Count -gt 0)
		{
			$FailedNames = @($FailedChecks | ForEach-Object { [string]$_.name })
			throw "Failed Unreal checks in $Path`: $($FailedNames -join ', ')"
		}
	}

	if ($Report.PSObject.Properties.Name -contains 'summary' -and
		$Report.summary.PSObject.Properties.Name -contains 'failed' -and
		[int]$Report.summary.failed -ne 0)
	{
		throw "Unreal report summary contains $($Report.summary.failed) failed checks: $Path"
	}

	return $Report
}

$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
$ProjectPath = Join-Path $ProjectRoot 'FactoryGame.uproject'
$BuildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$UnrealEditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$RemoteRunner = Join-Path $ProjectRoot 'run_via_remote.py'
$UnrealPyRoot = Join-Path $PSScriptRoot 'UnrealPy'
$AxisFixScript = Join-Path $UnrealPyRoot 'fix_sign_editor_axis_labels.py'
$NativeValidationScript = Join-Path $UnrealPyRoot 'validate_native_sign_widget.py'
$AxisFixReport = Join-Path $ProjectRoot 'exports\rss-axis-label-fix.json'
$NativeValidationReport = Join-Path $ProjectRoot 'exports\rss-native-sign-widget-validation.json'
$RuntimeWrapper = Join-Path $ProjectRoot '.agents\skills\test-satisfactory-mod-runtime\scripts\Invoke-SatisfactoryModRuntimeTest.ps1'

$RequiredPaths = @($ProjectPath, $BuildScript, $UnrealEditorCmd, $RemoteRunner, $UnrealPyRoot, $AxisFixScript, $NativeValidationScript)
foreach ($RequiredPath in $RequiredPaths)
{
	if (-not (Test-Path -LiteralPath $RequiredPath))
	{
		throw "Required path not found: $RequiredPath"
	}
}

$EffectiveInstall = [bool]($Install -or $Launch)
$EffectivePackage = [bool]($Package -or $EffectiveInstall)
if ($Launch -and [string]::IsNullOrWhiteSpace($LogPattern))
{
	throw '-Launch requires non-empty -LogPattern for stable runtime assertion.'
}
if ($EffectivePackage -and -not (Test-Path -LiteralPath $RuntimeWrapper -PathType Leaf))
{
	throw "Runtime test wrapper not found: $RuntimeWrapper"
}
if ($EffectiveInstall)
{
	$RunningGames = @(Get-Process -Name 'FactoryGameSteam-Win64-Shipping' -ErrorAction SilentlyContinue)
	if ($RunningGames.Count -gt 0)
	{
		$GameProcesses = @($RunningGames | ForEach-Object { "$($_.ProcessName) PID $($_.Id)" }) -join ', '
		throw "Install blocked by running game ($GameProcesses). Close it manually and rerun. This script never stops game processes."
	}
}

[void](Get-Command -Name $PythonExecutable -ErrorAction Stop)
$PythonScripts = @(Get-ChildItem -LiteralPath $UnrealPyRoot -Filter '*.py' -File | Sort-Object -Property Name)
if ($PythonScripts.Count -eq 0)
{
	throw "No Unreal Python scripts found below $UnrealPyRoot."
}

Write-Host "Checking Python syntax for $($PythonScripts.Count) Unreal scripts."
foreach ($PythonScript in $PythonScripts)
{
	Assert-PythonSyntax -Python $PythonExecutable -Script $PythonScript
}

$EditorBuildStatus = 'skipped'
$Editors = @(Get-RunningUnrealEditor)
if (-not $SkipEditorBuild)
{
	if ($Editors.Count -gt 0)
	{
		$EditorProcesses = @($Editors | ForEach-Object { "$($_.ProcessName) PID $($_.Id)" }) -join ', '
		throw "Development Editor build blocked by open editor ($EditorProcesses). It locks UnrealEditor-RSS.dll, so loaded DLL may be stale. Close editor and rerun, or use -SkipEditorBuild only after intended DLL is loaded. Existing editor will not be stopped."
	}

	Write-Host 'Building RSS Development Editor module.'
	& $BuildScript FactoryEditor Win64 Development "-Project=$ProjectPath" -WaitMutex -NoHotReloadFromIDE -Module=RSS
	Assert-NativeExitCode -Action 'RSS Development Editor build'
	$EditorBuildStatus = 'passed'
}
elseif ($Editors.Count -gt 0 -and -not $SkipUnrealValidation)
{
	Write-Warning 'Development Editor build skipped. Unreal validation uses DLL already loaded by editor; stale native API will fail validation.'
}

$UnrealValidationStatus = 'skipped'
if (-not $SkipUnrealValidation)
{
	$Editors = @(Get-RunningUnrealEditor)
	if ($Editors.Count -gt 1)
	{
		$EditorProcesses = @($Editors | ForEach-Object { "$($_.ProcessName) PID $($_.Id)" }) -join ', '
		throw "Multiple Unreal editors found ($EditorProcesses). Remote runner selects first node ambiguously; close extras without using this script."
	}
	if ($Editors.Count -eq 0)
	{
		if ($EditorBuildStatus -ne 'passed')
		{
			throw 'Unreal validation has no execution route: Development Editor build was skipped and no editor is running. Start one editor with intended DLL and rerun -SkipEditorBuild, or rerun without -SkipEditorBuild for headless validation.'
		}
		else
		{
			$AxisStartedAtUtc = [datetime]::UtcNow
			$AxisExitCode = Invoke-HeadlessUnrealScript -UnrealEditorCmd $UnrealEditorCmd -Project $ProjectPath -Script $AxisFixScript
			$AxisReport = Read-PassedJsonReport -Path $AxisFixReport -StartedAtUtc $AxisStartedAtUtc -RequireVerified
			if ($AxisExitCode -ne 0)
			{
				throw "Unreal headless axis-label script exited with code $AxisExitCode despite passed report."
			}
			Write-Host "Axis-label report passed through commandlet: $($AxisReport.asset)"

			$ValidationStartedAtUtc = [datetime]::UtcNow
			$ValidationExitCode = Invoke-HeadlessUnrealScript -UnrealEditorCmd $UnrealEditorCmd -Project $ProjectPath -Script $NativeValidationScript
			$NativeReport = Read-PassedJsonReport -Path $NativeValidationReport -StartedAtUtc $ValidationStartedAtUtc -RejectLimitations
			if ($ValidationExitCode -ne 0)
			{
				throw "Unreal headless native validation exited with code $ValidationExitCode despite passed report."
			}
			$CheckCount = if ($NativeReport.PSObject.Properties.Name -contains 'checks') { @($NativeReport.checks).Count } else { 0 }
			if ($CheckCount -eq 0)
			{
				throw "Native renderer report passed without behavior checks: $NativeValidationReport"
			}
			Write-Host "Native renderer report passed through commandlet: $CheckCount checks."
			$UnrealValidationStatus = 'passed-headless'
		}
	}
	else
	{
		$AxisStartedAtUtc = [datetime]::UtcNow
		$AxisExitCode = Invoke-RemoteUnrealScript -Python $PythonExecutable -RemoteRunner $RemoteRunner -Script $AxisFixScript
		$AxisReport = Read-PassedJsonReport -Path $AxisFixReport -StartedAtUtc $AxisStartedAtUtc -RequireVerified
		if ($AxisExitCode -ne 0)
		{
			throw "Unreal remote axis-label script exited with code $AxisExitCode despite passed report."
		}
		Write-Host "Axis-label report passed: $($AxisReport.asset)"

		$ValidationStartedAtUtc = [datetime]::UtcNow
		$ValidationExitCode = Invoke-RemoteUnrealScript -Python $PythonExecutable -RemoteRunner $RemoteRunner -Script $NativeValidationScript
		$NativeReport = Read-PassedJsonReport -Path $NativeValidationReport -StartedAtUtc $ValidationStartedAtUtc -RejectLimitations
		if ($ValidationExitCode -ne 0)
		{
			throw "Unreal remote native validation exited with code $ValidationExitCode despite passed report."
		}
		$CheckCount = if ($NativeReport.PSObject.Properties.Name -contains 'checks') { @($NativeReport.checks).Count } else { 0 }
		if ($CheckCount -eq 0)
		{
			throw "Native renderer report passed without behavior checks: $NativeValidationReport"
		}
		Write-Host "Native renderer report passed: $CheckCount checks."
		$UnrealValidationStatus = 'passed'
	}
}

$RuntimeStatus = 'skipped'
if ($EffectivePackage)
{
	$RuntimeArguments = @{
		PluginName = 'RSS'
		ProjectRoot = $ProjectRoot
		EngineRoot = $EngineRoot
		GameRoot = $GameRoot
		SteamExe = $SteamExe
		LogPattern = if ([string]::IsNullOrWhiteSpace($LogPattern)) { '__RSS_RUNTIME_ASSERTION_NOT_REQUESTED__' } else { $LogPattern }
		ExpectedCount = if ($Launch) { $ExpectedCount } else { 0 }
	}
	if (-not [string]::IsNullOrWhiteSpace($SavePath))
	{
		$RuntimeArguments.SavePath = $SavePath
	}
	if ($EffectiveInstall)
	{
		$RuntimeArguments.Install = $true
	}
	if ($Launch)
	{
		$RuntimeArguments.Launch = $true
	}

	Write-Host "Delegating RSS package/runtime stage (Install=$EffectiveInstall, Launch=$([bool]$Launch))."
	& $RuntimeWrapper @RuntimeArguments
	Assert-NativeExitCode -Action 'RSS package/runtime wrapper'
	$RuntimeStatus = if ($Launch) { 'runtime-passed' } elseif ($EffectiveInstall) { 'installed' } else { 'packaged' }
}

[pscustomobject]@{
	ProjectRoot = $ProjectRoot
	PythonScriptsChecked = $PythonScripts.Count
	EditorBuild = $EditorBuildStatus
	UnrealValidation = $UnrealValidationStatus
	Runtime = $RuntimeStatus
	Installed = $EffectiveInstall
	Launched = [bool]$Launch
}
