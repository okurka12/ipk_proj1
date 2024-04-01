# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased] - yyyy-mm-dd

Here are limitations of the current implementation that I know of.

### To do

- send ERR messages to the server whend it sends invalid data
- check that all pointer parameters to functions have a const qualifier unless
the functions write to the target
- try to send BYE when receiving ERR (right now the ERR is only printed and
the program carries on like nothing happended)
- reduce the number of `malloc` calls by `udp_print_msg` module

## [1.0.0] - 2024-04-01

This is my final version of the project I'll be submitting.

### Added

- Functionality for UDP communication
- Functionality for TCP communication
- Functionality for standard input processing
- Functionality for gracefully exiting upon SIGINT
