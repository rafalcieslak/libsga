param (
    [Parameter(Mandatory=$true)][string]$version
)

write-output "Preparing Windows release"
write-host   "Preparing Windows release HOST"
write-output "Preparing Windows release for $version"

$basedir = "release-windows"
$reldir = "libsga-$version-windows"
$sdkdir = "$basedir\$reldir"

Remove-Item -Recurse -Force $sdkdir
New-Item -ItemType Directory -Force -Path "$sdkdir"
New-Item -ItemType Directory -Force -Path "$sdkdir\include"

# Main library
Copy-Item "Release\sga.dll" $sdkdir

# Header files
#$sourcePath = '..\include'
#$destPath = '$sdkdir\include\'
#Get-ChildItem $sourcePath -Recurse -Include '*.hpp', '*.inc' | Foreach-Object `
#  {
#      $destDir = Split-Path ($_.FullName -Replace [regex]::Escape($sourcePath), $destPath)
#      if (!(Test-Path $destDir))
#      {
#          New-Item -ItemType directory $destDir | Out-Null
#      }
#      Copy-Item $_ -Destination $destDir
#  }
Copy-Item -Path "..\include" -Filter "*.txt" -Recurse -Destination "$sdkdir\include" -Container

# HTML docs
Copy-Item -Path "doc/html" -Recurse -Destination "$sdkdir\doc" -Container

# TODO: Example files

# Create release archive
function Zip-Files( $zipfilename, $sourcedir )
{
   Add-Type -Assembly System.IO.Compression.FileSystem
   $compressionLevel = [System.IO.Compression.CompressionLevel]::Optimal
   [System.IO.Compression.ZipFile]::CreateFromDirectory($sourcedir,
        $zipfilename, $compressionLevel, $false)
}
Zip-Files "$RELDIR.zip" "$RELDIR"

write-output "Done Windows release"
write-host   "Done Windows release HOST"
