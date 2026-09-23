# Convert the supplied Image2LCD RGB565 C arrays to the 110x110 LCD icon size.
# Run from any directory; generated arrays contain pixels only (no 8-byte header).
$ErrorActionPreference = 'Stop'
$sourceDir = Join-Path $PSScriptRoot '..\..\resource\image\c'
$outputDir = Join-Path $PSScriptRoot '..\User\image\weather'
$icons = @(
    'baoxue', 'baoyu', 'dafeng', 'daxue', 'dayu', 'duoyun',
    'leizhenyu', 'mai', 'shachen', 'wu', 'xiaoxue', 'xiaoyu',
    'yin', 'yujiaxue', 'zhenyu', 'zhongxue', 'zhongyu'
)
$targetSize = 110
[IO.Directory]::CreateDirectory($outputDir) | Out-Null

foreach ($icon in $icons) {
    $inputPath = Join-Path $sourceDir "icon_$icon.c"
    $inputText = [IO.File]::ReadAllText($inputPath)
    $declaration = [regex]::Match($inputText, 'const\s+unsigned\s+char\s+\w+\[(\d+)\]\s*=\s*\{')
    if (-not $declaration.Success) { throw "Array declaration missing: $inputPath" }
    $declaredLength = [int]$declaration.Groups[1].Value
    $body = $inputText.Substring($declaration.Index + $declaration.Length)
    $metadata = [regex]::Match($body, '^\s*/\*\s*((?:0X[0-9A-Fa-f]{2},?\s*){8})\*/')
    if (-not $metadata.Success) { throw "Image2LCD dimensions missing: $inputPath" }
    $header = [regex]::Matches($metadata.Groups[1].Value, '0X([0-9A-Fa-f]{2})')
    $sourceWidth = [Convert]::ToInt32($header[2].Groups[1].Value, 16) +
                   256 * [Convert]::ToInt32($header[3].Groups[1].Value, 16)
    $sourceHeight = [Convert]::ToInt32($header[4].Groups[1].Value, 16) +
                    256 * [Convert]::ToInt32($header[5].Groups[1].Value, 16)
    $pixels = [regex]::Matches($body.Substring($metadata.Length), '0X([0-9A-Fa-f]{2})')
    if ($declaredLength -ne $sourceWidth * $sourceHeight * 2 -or
        $pixels.Count -ne $declaredLength) {
        throw "Unexpected pixel count in $inputPath ($($pixels.Count), expected $declaredLength)"
    }

    $out = [Text.StringBuilder]::new()
    [void]$out.AppendLine("/* Generated from resource/image/c/icon_$icon.c; RGB565 little-endian, 110x110. */")
    [void]$out.AppendLine("const unsigned char gWeather_$icon[$($targetSize * $targetSize * 2)] = {")
    for ($y = 0; $y -lt $targetSize; $y++) {
        $sourceY = [int][Math]::Floor($y * $sourceHeight / $targetSize)
        for ($x = 0; $x -lt $targetSize; $x++) {
            $sourceX = [int][Math]::Floor($x * $sourceWidth / $targetSize)
            $offset = 2 * ($sourceY * $sourceWidth + $sourceX)
            if ((($y * $targetSize + $x) % 8) -eq 0) { [void]$out.Append('    ') }
            [void]$out.Append('0x').Append($pixels[$offset].Groups[1].Value).Append(', ')
            [void]$out.Append('0x').Append($pixels[$offset + 1].Groups[1].Value).Append(',')
            if ((($y * $targetSize + $x) % 8) -eq 7) { [void]$out.AppendLine() }
            else { [void]$out.Append(' ') }
        }
    }
    if (($targetSize * $targetSize) % 8 -ne 0) { [void]$out.AppendLine() }
    [void]$out.AppendLine('};')
    $outputPath = Join-Path $outputDir "icon_$icon.c"
    [IO.File]::WriteAllText($outputPath, $out.ToString(), [Text.UTF8Encoding]::new($false))
    Write-Output "$icon : ${sourceWidth}x${sourceHeight} -> ${targetSize}x${targetSize}"
}
