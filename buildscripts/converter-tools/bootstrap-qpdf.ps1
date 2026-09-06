# The production entry point exposes only provisioning inputs. Fault barriers
# live in the module's test API and are never accepted from the command line.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DestinationRoot,
    [string]$CacheRoot = (Join-Path $PSScriptRoot '../../build.tools/downloads'),
    [switch]$ForceReinstall
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'qpdfbootstrap.psm1') -Force
try {
    Invoke-QpdfBootstrap -DestinationRoot $DestinationRoot -CacheRoot $CacheRoot -ForceReinstall:$ForceReinstall
} catch {
    Write-Error $_
    exit 1
}
