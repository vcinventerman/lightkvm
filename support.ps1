# Valid commands:
# gdb COMX - Restart target into GDB-Stub

#todo:
# allow use of Crtl-C in loops

if ($args[0] -eq "gdb") {
    $port = $args[1]

    $conn = new-Object System.IO.Ports.SerialPort $port, 115200, None, 8, one
    $conn.open()

    # Reset chip over EN
    $conn.RtsEnable = $true;
    Start-Sleep 0.5
    $conn.RtsEnable = $false;
    Start-Sleep 0.5

    # Wait for app_main to run
    do {
        $buf = $conn.ReadLine();
        echo "Got data $($buf)"
    } while ($buf.Trim() -ne "!!! init complete")

    $conn.Write("\x03")

    # Wait for ESP32 to send "$T0b#e6" as a response to GDB-Stub starting
    Start-Sleep 0.5

    $conn.Close()
}