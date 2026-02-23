$times = @()
for ($i = 0; $i -lt 5; $i++) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & .\mandelbrot.exe | Out-Null
    $sw.Stop()
    $times += $sw.ElapsedMilliseconds
    Write-Host "Run $($i+1): $($sw.ElapsedMilliseconds) ms"
}
$min = ($times | Measure-Object -Minimum).Minimum
$max = ($times | Measure-Object -Maximum).Maximum
$avg = ($times | Measure-Object -Average).Average
Write-Host ""
Write-Host "--- benchmark (5 runs) ---"
Write-Host "  min: $min ms"
Write-Host "  avg: $([math]::Round($avg, 1)) ms"
Write-Host "  max: $max ms"
