#Requires AutoHotkey v2
#SingleInstance Force

port := "\\.\COM3"  ; Windows-spezifisch für COM3
baud := 9600

h := FileOpen(port, "r")
if !h {
    MsgBox("Fehler: COM-Port konnte nicht geöffnet werden!")
    ExitApp
}

SetTimer(CheckSerial, 50)
return

CheckSerial(*) {
    global h
    if !h {
        return
    }
    line := h.ReadLine()
    if line && StrTrim(line) = "PLAY" {
        Send("{Media_Play_Pause}")
    }
}
