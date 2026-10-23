# Limbo
Limbo is an IRC bot with the core functionality of bridging two IRC-Channels. **Currently, the bot only runs on Linux.**

## Build
Dependencies: Make, Gcc, OpenSSL, Git

```bash
    git clone git@github.com:TnKvDev/Limbo.git
    cd Limbo
    mkdir build
    make
```

## Configuring
Before running the Limbo bot, you should configure it first:

```bash
    cp boilerplate.ini conf.ini
    vi config.ini
```

Make sure you give the bot the necessary information about the IRC Networks you want to bridge.

## Running the bot
```bash
    make run
```
