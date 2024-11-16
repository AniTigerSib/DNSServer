# DNSServer

C++ realization of DNS emulator. Developed for SurSU Digital Challenge competitions.

## Team Information

**UB creators** team.

## Dependencies

- Boost library (asio for sockets)
- yaml-cpp for configuration purposes

## Install and Startup guide

#### Install

    mkdir build && cd build
    cmake ..
    make

## Configure

Configuration file has to be in yaml format and has to contains folowing fields:

- `log_filename` - Path to log file and base name;
- `logfile_size` - Maximum log file size (in kilobytes);
- `port` - Port number, program to be started on;
- `dns_server` - Preferred DNS server.

It may looks like this:

    log_filename: "application.log"
    logfile_size: 512
    port: 8080
    dns_server: "8.8.8.8"

#### Startup

    ./DNSServer <path_to_config>

## Todo

Empty, finally... ;)

I know, it needs to get logging system, specialized for such type of software, but... I'm done, really...

P.S. And maybe it needs some good documentation, but I'm not shure, that I probably can write really good documentation after almost 16 hours of coding... Sorry, tutors, judges, observers, and every other, who reads this repository.