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

#include "node.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace {

using BackingStoreMap = std::map<std::string, std::shared_ptr<v8::BackingStore>>;

union Storage {
    BackingStoreMap map;
    Storage() {}
    ~Storage() {}
} storage;

// |map| is a pointer to storage.map because BackingStore destructors should
// not run on program exit, by that time V8 is dead
BackingStoreMap* map;
std::mutex mtx;

void Throw(v8::Isolate* isolate, const char* message)
{
    auto arg = v8::String::NewFromUtf8(isolate, message).ToLocalChecked();
    auto exc = v8::Exception::TypeError(arg);
    isolate->ThrowException(exc);
}

void GetOrCreate(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    v8::String::Utf8Value key(isolate, args[0]);
    if (!*key) return; // exception pending
    std::lock_guard<std::mutex> lock(mtx);
    auto it = map->find(*key);
    v8::Local<v8::SharedArrayBuffer> result;
    if (it == map->end()) {
        if (!args[1]->IsFunction()) return Throw(isolate, "not a function");
        auto undefined = v8::Undefined(isolate);
        auto context = isolate->GetCurrentContext();
        auto fun = args[1].As<v8::Function>();
        v8::Local<v8::Value> rval;
        if (!fun->Call(context, undefined, 0, nullptr).ToLocal(&rval)) return; // exception pending
        if (!rval->IsSharedArrayBuffer()) return Throw(isolate, "not a SharedArrayBuffer");
        result = rval.As<v8::SharedArrayBuffer>();
        map->insert(std::make_pair(std::string(*key), result->GetBackingStore()));
    } else {
        result = v8::SharedArrayBuffer::New(isolate, it->second);
    }
    args.GetReturnValue().Set(result);
}

void Remove(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();
    v8::String::Utf8Value key(isolate, args[0]);
    if (!*key) return; // exception pending
    std::lock_guard<std::mutex> lock(mtx);
    auto it = map->find(*key);
    auto found = (it != map->end());
    if (found) map->erase(it);
    args.GetReturnValue().Set(found);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> module_,
                v8::Local<v8::Context> context,
                void* unused)
{
    auto isolate = v8::Isolate::GetCurrent();
    auto undefined = v8::Undefined(isolate);
    {
        auto key = v8::String::NewFromUtf8Literal(isolate, "getOrCreate");
        auto value = v8::Function::New(context, GetOrCreate, undefined, /*length*/2).ToLocalChecked();
        exports->Set(context, key, value).Check();
    }
    {
        auto key = v8::String::NewFromUtf8Literal(isolate, "remove");
        auto value = v8::Function::New(context, Remove, undefined, /*length*/1).ToLocalChecked();
        exports->Set(context, key, value).Check();
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!map) map = new(&storage.map) BackingStoreMap();
    }
}

NODE_MODULE_CONTEXT_AWARE(NODE_GYP_MODULE_NAME, Initialize)

}
