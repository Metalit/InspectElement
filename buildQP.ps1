# Builds a .qmod file for loading with QP
& $PSScriptRoot/build.ps1

$ArchiveName = "Inspect Element.qmod"
$TempArchiveName = "Inspect Element.qmod.zip"

Compress-Archive -Path "./libs/arm64-v8a/libInspectElement.so", ".\extern\libbeatsaber-hook_2_3_2.so", ".\mod.json" -DestinationPath $TempArchiveName -Force
Move-Item $TempArchiveName $ArchiveName -Force