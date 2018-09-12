Minimal v8 implementation of an inspector to try and get chrome dev tools to work!

The only real dependency is v8 and soylib (and that's only for my sockets/websocket stuff, and a few array, string things, which could be stripped out)

Here is where I'm trying to get chrome dev tools to work and some debug.
https://groups.google.com/forum/?nomobile=true#!topic/v8-users/L8rq6zbviHg

Guide:
===============
- Install xcode.
- Open XCode.
- Build V8.
- Drop libs, includes, and runtime into /src/v8/ 
