param (
    [Parameter(Mandatory=$true)][string]$version
)

write-output "Preparing Windows release for $version"

$basedir = "release-windows"
$reldir = "libsga-$version-windows"
$sdkdir = "$basedir\$reldir"

Remove-Item $sdkdir -Recurse -ErrorAction Ignore
New-Item "$sdkdir" -ItemType Directory
New-Item "$sdkdir\include" -ItemType Directory

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
Copy-Item -Path "doc\html" -Recurse -Destination "$sdkdir\doc" -Container

# TODO: Example files

# Create release archive
function Zip-Files( $zipfilename, $sourcedir )
{
    # ZipFile.CreateFromDirectory is completely broken. See
    # https://gist.github.com/rafalcieslak/034b47f63abf81fa7ffc5992f723f814
    # for details.
    Add-Type -AssemblyName System.Text.Encoding
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    class FixedEncoder : System.Text.UTF8Encoding {
        FixedEncoder() : base($true) { }

        [byte[]] GetBytes([string] $s)
        {
            write-output $s
            $s = $s.Replace("\\", "/");
            write-output $s
            return ([System.Text.UTF8Encoding]$this).GetBytes($s);
        }
    }

   Add-Type -Assembly System.IO.Compression.FileSystem
   $compressionLevel = [System.IO.Compression.CompressionLevel]::Optimal
   [System.IO.Compression.ZipFile]::CreateFromDirectory($sourcedir, $zipfilename, $compressionLevel, $false, [FixedEncoder]::new())
}
Zip-Files "$reldir.zip" "$sdkdir"

write-output "Done Windows release"
write-host   "Done Windows release HOST"
