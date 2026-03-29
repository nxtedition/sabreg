sabreg
======

A multi-threaded registry for SharedArrayBuffers.

Usage:
```js
const sab = sabreg.getOrCreate("my-first-sab", () => new SharedArrayBuffer(42))
// do something with |sab|
```
The registry retains strong references to SharedArrayBuffers so the last
thread should remove them:
```js
sabreg.remove("my-first-sab")
```
