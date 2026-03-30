sabreg
======

A multi-threaded registry for SharedArrayBuffers.

Usage:
```js
const sab = sabreg.getOrCreate("my-first-sab", () => new SharedArrayBuffer(42))
// do something with |sab|
```
Keys are strong references, SharedArrayBuffer values (or rather, their backing
stores) are stored as weak references. If you have lots of keys, remove them
manually:
```js
sabreg.remove("my-first-sab")
```
