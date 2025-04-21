# tos-rtd-c
Thinkorswim Real Time Data in C

A lightweight COM client for accessing real-time market data from ThinkOrSwim's RTD server.

![image](https://github.com/user-attachments/assets/d0849574-ccdf-43e2-811b-0d92f2e9854d)

## Requirements

- **Windows Operating System** - This application uses Windows-specific COM technology
- **ThinkOrSwim Desktop Application** - Must be running for the RTD server to be available
- **Visual Studio or LLVM Clang or MinGW GCC** - For compiling the application

## Compilation

To compile the application:

```
clang -Wall -O2 -o rtd_client_clang.exe rtd_client.c -lole32 -loleaut32 -luuid -luser32 -DUNICODE -D_UNICODE

gcc -Wall -O2 -o rtd_client.exe rtd_client.c -lole32 -loleaut32 -luuid -luser32 -DUNICODE -D_UNICODE

Command Line (Developer Command Prompt):
cl rtd_client.c /O2 /W4 /DUNICODE /D_UNICODE /link ole32.lib oleaut32.lib uuid.lib user32.lib
```

## Features

- Connect to ThinkOrSwim's RTD server to receive real-time market data
- Track various data topics (LAST, BID, ASK, VOLUME, etc.)

## Usage

1. Start the ThinkOrSwim desktop application
2. Run the RTD client executable
3. Enter symbol and topic.
4. At anytime press "Enter" to change symbol or exit.

### Example Topics

- `LAST` - Last trade price
- `BID` - Current bid price
- `ASK` - Current ask price
- `VOLUME` - Trading volume
- `LAST_SIZE` - Size of last trade
- `GAMMA`- Option gamma

## Example Session

```
RTD Client - Real-time Market Data Viewer

Enter stock symbol: AAPL
Enter data topic (LAST, BID, ASK, etc): LAST

[13:45:22.124] AAPL = 167.28

```


