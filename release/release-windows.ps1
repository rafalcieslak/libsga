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
Copy-Item -Path "..\include" -Include "*.hpp","*.inc" -Recurse -Destination "$sdkdir" -Container

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
    $EncoderClass=@"
      public class FixedEncoder : System.Text.UTF8Encoding {
        public FixedEncoder() : base(true) { }
        public override byte[] GetBytes(string s) {
        s = s.Replace("\\", "/");
        return base.GetBytes(s);
      }
    }
"@
    Add-Type -TypeDefinition $EncoderClass
    $Encoder = New-Object FixedEncoder
    $compressionLevel = [System.IO.Compression.CompressionLevel]::Optimal
    [System.IO.Compression.ZipFile]::CreateFromDirectory($sourcedir, $zipfilename, [System.IO.Compression.CompressionLevel]::Optimal, $false, $Encoder)
}
Zip-Files "$reldir.zip" "$sdkdir"
Copy-Item -Path "$reldir.zip" -Destination "../$reldir.zip"
