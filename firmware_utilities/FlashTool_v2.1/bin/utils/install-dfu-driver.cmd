@ECHO OFF

CD %1

where /q pnputil
IF ERRORLEVEL 1 (
    ECHO Application pnputil is missing! Can not remove bloated oemXX.inf drivers.
) ELSE (
    powershell -Command "pnputil /e | Select-String -Context 2 'Driver package provider :\s+ libwdi' | ForEach-Object { ($_.Context.PreContext[1] -split ' : +')[1] } | ForEach-Object {pnputil /d $_}"
)
powershell -Command "If ((Get-WindowsDriver -Online | where ProviderName -match 'libwdi' | where Version -match '6.1.7600.16385').Count -gt 0) {exit 0} else {exit 1}"
IF ERRORLEVEL 1 (
    ECHO Installing libwdi DFU driver...
    zadic.exe --vid 0x0483 --pid 0xDF11 --create stm32dfu
) ELSE (
    ECHO Libwdi DFU driver is allready installed.
)
