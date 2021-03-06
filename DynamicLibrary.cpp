/*
 * Copyright (c) 2012, Aaron Coombs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Autohub++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL AARON COOMBS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include "include/DynamicLibrary.hpp"
#include "include/autoapi.hpp"

#include <sstream>
#include <iostream>

namespace ace {

DynamicLibrary::DynamicLibrary(void * handle) : handle_(handle){
}

DynamicLibrary::~DynamicLibrary() {
    if (handle_) {
#ifndef WIN32
        ::dlclose(handle_);
#else
        ::FreeLibrary((HMODULE) handle_);
#endif
    }
}

std::shared_ptr<DynamicLibrary>
DynamicLibrary::load(const std::string & name,
        std::string & errorString) {
    if (name.empty()) {
        errorString = "Empty path.";
        return nullptr;
    }

    void * handle = nullptr;

#ifdef WIN32
    handle = ::LoadLibraryA(name.c_str());
    if (handle == nullptr) {
        DWORD errorCode = ::GetLastError();
        std::stringstream ss;
        ss << std::string("LoadLibrary(") << name
                << std::string(") Failed. errorCode: ")
                << errorCode;
        errorString = ss.str();
    }
#else
    handle = ::dlopen(name.c_str(), RTLD_NOW);
    if (!handle) {
        std::string dlErrorString;
        const char *zErrorString = ::dlerror();
        if (zErrorString)
            dlErrorString = zErrorString;
        errorString += "Failed to load \"" + name + '"';
        if (dlErrorString.size())
            errorString += ": " + dlErrorString;
        return nullptr;
    }

#endif
    return std::make_shared<DynamicLibrary>(handle);
}

void *
DynamicLibrary::getSymbol(const std::string & symbol) {
    if (!handle_)
        return nullptr;

#ifdef WIN32
    return ::GetProcAddress((HMODULE) handle_, symbol.c_str());
#else
    return ::dlsym(handle_, symbol.c_str());
#endif
}

std::shared_ptr<AutoAPI>
DynamicLibrary::get_object() {
    if (api_)
        return api_;
    return nullptr;
}

void
DynamicLibrary::set_object(std::shared_ptr<AutoAPI> api) {
    if (api_)
        api_.reset();
    api_ = api;
}
} // namespace ace
