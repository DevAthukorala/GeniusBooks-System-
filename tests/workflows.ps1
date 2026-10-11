param([Parameter(Mandatory=$true)][string]$Executable)
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path $Executable).Path
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('geniusbooks-tests-' + [guid]::NewGuid())
New-Item -ItemType Directory $testRoot | Out-Null
$data = Join-Path $testRoot 'shop.data'
function Run-App([string[]]$Lines, [string]$Path = $data, [int]$ExpectedExit = 0) {
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $exe
    $info.Arguments = '"' + $Path + '"'
    $info.UseShellExecute = $false
    $info.RedirectStandardInput = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($info)
    $output = $process.StandardOutput.ReadToEndAsync()
    $errorOutput = $process.StandardError.ReadToEndAsync()
    foreach ($line in $Lines) { $process.StandardInput.WriteLine($line) }
    $process.StandardInput.Close()
    if (!$process.WaitForExit(10000)) { $process.Kill(); throw 'Application timed out' }
    $text = $output.Result + $errorOutput.Result
    if ($process.ExitCode -ne $ExpectedExit) { throw "Unexpected exit: $text" }
    $process.Dispose()
    return $text
}
function Has([string]$Text, [string]$Expected) {
    if (!$Text.Contains($Expected)) { throw "Missing '$Expected' in output:`n$Text" }
}
Has (Run-App @('2','0')) 'No books found.'
$output = Run-App @('1','The "Book"','Writer','bad','-1','12.345','0.10','-2','5','3','BOOK','5','1','6','2','y','6','0')
Has $output 'Enter a price'
Has $output 'Enter a whole number from 0 to 5'
Has $output 'Sale total: 0.20'
Has $output 'Revenue: 0.20'
$output = Run-App @('2','4','1','1','Renamed','4','1','2','New Author','4','1','3','9.99','4','1','4','7','3','new author','6','0')
Has $output 'Stock: 3'
Has $output 'Renamed | New Author | Price: 9.99 | Stock: 7'
Has $output 'The "Book" | Qty: 2 | Unit price: 0.10'
$before = [IO.File]::ReadAllText($data)
Has (Run-App @('5','1','2','n','0')) 'Sale cancelled.'
if ([IO.File]::ReadAllText($data) -ne $before) { throw 'Cancellation changed data' }
Run-App @('1','Incomplete') | Out-Null
if ([IO.File]::ReadAllText($data) -ne $before) { throw 'EOF changed data' }
Has (Run-App @('4','999','0')) 'Book not found.'
Has (Run-App @('4','1','4','0','5','1','0')) 'Out of stock.'
# A directory at the temporary file path forces a save failure.
New-Item -ItemType Directory ($data + '.tmp') | Out-Null
$output = Run-App @('4','1','4','99','2','0')
Has $output 'Change not saved:'
Has $output 'Stock: 0'
$bad = Join-Path $testRoot 'corrupt.data'
[IO.File]::WriteAllText($bad, "GENIUSBOOKS 1`nB broken`n")
Has (Run-App @() $bad 1) 'Corrupt data record'
if ([IO.File]::ReadAllText($bad) -ne "GENIUSBOOKS 1`nB broken`n") { throw 'Corrupt data was overwritten' }
$lock = [IO.File]::Open($data + '.lock', [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
try { Has (Run-App @() $data 1) 'Data is in use' } finally { $lock.Dispose() }
Write-Output "All workflow tests passed. Test data: $testRoot"
