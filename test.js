// Copyright (c) 2026, nxtedition
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

"use strict"

const assert = require("node:assert")
const sabreg = require("./")
const worker_threads = require("node:worker_threads")

assert.equal(true, worker_threads.isMainThread)

assert.throws(() => {
    sabreg.getOrCreate("foo")
}, /not a function/)

assert.throws(() => {
    sabreg.getOrCreate("foo", () => {})
}, /not a SharedArrayBuffer/)

assert.throws(() => {
    sabreg.getOrCreate("foo", () => { throw Error("kaboom") })
}, /kaboom/)

{
    const expected = new SharedArrayBuffer(4)
    const actual = sabreg.getOrCreate("foo", () => expected)
    assert.equal(expected, actual)

    // calling it again returns a new SharedArrayBuffer but with the same backing store
    const other = sabreg.getOrCreate("foo", () => { throw Error("unreachable") })
    assert.notEqual(other, expected)

    // check |other| and |actual| share their backing store
    assert.equal(0, new Uint32Array(other)[0])
    new Uint32Array(actual)[0] = 42
    assert.equal(42, new Uint32Array(other)[0])
    assert.equal(true, sabreg.remove("foo"))
    assert.equal(false, sabreg.remove("foo"))

    // trying to create it again should invoke the callback
    assert.throws(() => {
        sabreg.getOrCreate("foo", () => { throw Error("kaboom") })
    }, /kaboom/)
}

{
    // worker creates a SharedArrayBuffer
    const worker = new worker_threads.Worker("./test_worker_thread.js")
    let called = 0
    process.on("exit", () => assert.equal(called, 1))
    worker.on("exit", (code) => {
        called++
        assert.equal(0, code)
        const actual = sabreg.getOrCreate("bar", () => { throw Error("unreachable") })
        assert.equal(true, sabreg.remove("bar"))
        assert.equal(1337, new Uint32Array(actual)[0])
    })
}
