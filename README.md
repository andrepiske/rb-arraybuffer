
# arraybuffer gem

[![CircleCI](https://circleci.com/gh/andrepiske/rb-arraybuffer.svg?style=shield)](https://circleci.com/gh/andrepiske/rb-arraybuffer)
[![Gem Version](https://badge.fury.io/rb/arraybuffer.svg)](https://badge.fury.io/rb/arraybuffer)

# What?

Ruby lacks classes like array buffer or byte array.

This gem aims to solve that by implementing that natively in C so that
a decent performance can be achieved.

The design of the classes follows standards that are implemented in the
Web world, like:

- [DataView](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView)
- [ArrayBuffer](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer)

The standards above are, however, not strictly followed.

# Why?

The only way I found to do this was manipulating an array of numbers
treating them as bytes and using `buffer.pack('C*')` to transform it
into a `String` object. However, for some reason the `Array#pack` method
is painfully slow.

Other gems exist out there, like the class
[ByteBuffer](https://www.rubydoc.info/gems/nio4r/2.5.2/NIO/ByteBuffer)
class in [nio4r](https://rubygems.org/gems/nio4r). However, they
didn't had a design I was satisfied with or they had different purposes
that wouldn't suit my use case :)

# What about JRuby?

Feel free to open a pull request for that!
