# Start gateway server and ngrok tunnel
Write-Host "Starting gateway server..." -ForegroundColor Green
$serverProcess = Start-Process powershell -ArgumentList "-NoExit", "-Command", "cd '$PSScriptRoot\gateway-server'; npm start" -PassThru -WindowStyle Minimized

Write-Host "Waiting for server to start..." -ForegroundColor Yellow
Start-Sleep -Seconds 5

Write-Host "Starting ngrok tunnel..." -ForegroundColor Green
Write-Host "ngrok will expose http://localhost:3000" -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop ngrok" -ForegroundColor Yellow
Write-Host ""

# Start ngrok
ngrok http 3000

