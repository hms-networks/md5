# MD5

This is a basic project that contains an implementation of the RFC1321 defined
MD5 hashing algorithm. This unit supports multiple simultaneous computations
of MD5 hashes via the use an MD5 instance handle that tracks the state
independently from other instances. The unit accepts taking in subsets of the
overall payload lending itself well to stream based processing.

This implementation is a derivative implementation based on several sources.
Namely the MD5 algorithm described in RFC1321, the pseudocode from Wikipedia,
and an implementation from rosettacode.

## Why Another MD5 Implementation?

Existing MD5 implementations appear to be more file-system oriented and lack
a focus on portability which is arguably an important feature for embedded
applications.

The goal of this project is to have a portable, low-dependency, small-footprint
implementation. This includes support for microcontrollers that use 16-bit
characters (i.e. they are not byte addressable). A future update to this project
would ideally ensure compatibility with big-endian systems.

## Example Application

MD5_example_app.c contains an example Windows console application that allows
the user to specify a file to compute the MD5 result for. Various optional
arguments exists to allow for validating against a supplied MD5-file, or
benchmarking the process using a variety of read-sizes.

## Credit

- tools.ietf.org/html/rfc1321
- rosettacode.org/wiki/MD5#C
- en.wikipedia.org/wiki/MD5
