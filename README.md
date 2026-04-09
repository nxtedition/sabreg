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
Be careful to retain at least one strong reference if you need the
SharedArrayBuffer in the future. Consider this superficially correct-looking
but actually buggy example:
```js
function create() {
    if (create.count++) throw "again?"
    return new SharedArrayBuffer(42)
}
create.count = 0

// `a` looks like a strong reference, however...
const a = sabreg.getOrCreate("foo", create)

// for expository reasons only; can also happen implicitly
global.gc()

// may or may not throw!
const b = sabreg.getOrCreate("foo", create)
```
The garbage collector can reclaim the SharedArrayBuffer because there are
no uses of `a` past the second call to `getOrCreate()`, i.e., `a` does not
live long enough.

A simple workaround is to store a reference in a global variable:
```js
// also store a reference in a global variable
const a = globalThis.a = sabreg.getOrCreate("foo", create)

// safe!
const b = sabreg.getOrCreate("foo", create)

delete globalThis.a
```
Note that trying to artificially extend the lifespan of `a` is not sound:
```js
// let binding that we reassign below
let a = sabreg.getOrCreate("foo", create)

// may or may not throw
const b = sabreg.getOrCreate("foo", create)

// V8 can reorder this with the getOrCreate call because that would normally
// be an unobservable change (a change that doesn't affect program evaluation)
a = null
```
