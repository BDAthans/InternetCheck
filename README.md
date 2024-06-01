# InternetCheck

InternetCheck is an extremely lightweight standalone application written in C++ designed to monitor internet connectivity by running discreetly in the background. It logs each instance when the internet connection is lost and restored, recording these events to a log file.

## Description

InternetCheck uses DNS lookups and ICMP (ping) requests to monitor the status of internet connectivity. It targets five key domains for these checks: google.com, microsoft.com, yahoo.com, amazon.com, and cisco.com. The application utilizes the Windows API for executing DNS and ICMP requests, ensuring compatibility exclusively with Windows operating systems.

The application is self-contained with all necessary libraries statically linked, thus requiring no additional installations.

## Installation & Execution

To install InternetCheck:
1. Place the `Install-Startup-RegKey.bat` file in the same directory as `InternetCheck.exe`.
2. Execute `Install-Startup-RegKey.bat` to configure the application to automatically start upon system login or boot.

To manually start the application, simply run `InternetCheck.exe`.

## Log Location

The application outputs its logs to `InternetCheck-Log.txt`, located in the same directory as the executable. This log file is appended with timestamps detailing when the internet connection goes down and comes back up.
