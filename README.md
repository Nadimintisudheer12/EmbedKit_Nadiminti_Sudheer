# EmbedKit_Nadiminti_Sudheer

## Name
Sudheer Nadiminti

## Module
UART Frame Parser

## Description
UART frame parser implemented using a finite state machine with checksum verification and timeout handling.

## Build

```bash
gcc -Wall -std=c99 uart_parser.c -o uart_parser
```

## Run

```bash
./uart_parser
```

## Features

- Byte-by-byte frame parsing
- Finite state machine implementation
- XOR checksum verification
- Configurable inter-byte timeout
- Automatic recovery after timeout
- Back-to-back frame support
- Timeout disable support
