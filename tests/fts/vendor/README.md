# Vendored reference implementation

`adafruit_ble_file_transfer.py` is copied unmodified from
[adafruit/Adafruit_CircuitPython_BLE_File_Transfer](https://github.com/adafruit/Adafruit_CircuitPython_BLE_File_Transfer)
at commit `14c0870bc915aba90cf6e8a4002adeb563bc95fe` (MIT, Scott Shawcroft for
Adafruit Industries; SPDX header in the file). `test_fts.py` drives its
`FileTransferClient` against `fts_model.py` and the C++ engine as an independent check of the
wire format; nothing else imports it.
