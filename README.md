# InternetCheck
C++ standalone application that runs in the background and logs when the internet goes down and back up to a log file.

## Description

In order to determine if the internet is down, the application performs DNS lookups and pings to 5 domains: google.com, microsoft.com, yahoo.com, amazon.com and cisco.com.
The DNS lookups and the ICMP requests rely on the Windows API. This application will run on Windows only.

There are no dependencies that need to be installed, as they are static linked within the application.

## Installation & Execution
Place the Install-Startup-RegKey.bat file into the same directory as the InternetCheck.exe, and then run Install-Startup-RegKey.bat to set the application to start on login/boot.

Manually the InternetCheck.exe can be started by running it.

## Log Location
The log file 'InternetCheck-Log.txt' will be generated and appended to when the internet goes up or down. 
