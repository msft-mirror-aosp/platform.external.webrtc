load("//:build_defs.bzl", _platform_select = "platform_select", _webrtc_grpc_library = "webrtc_grpc_library", _webrtc_proto_library = "webrtc_proto_library")

platform_select = _platform_select
webrtc_proto_library = _webrtc_proto_library
webrtc_grpc_library = _webrtc_grpc_library

def setup_webrtc_build():
    pass

def rtc_rust_cxx_bridge(name, **kwargs):
    native.filegroup(name = name)

def rtc_rust_unittest(name, **kwargs):
    native.filegroup(name = name)

def rtc_rust_test_suite(name, **kwargs):
    native.filegroup(name = name)

def java_proto_library(name, **kwargs):
    native.filegroup(name = name)

def android_binary(name, **kwargs):
    native.filegroup(name = name)

def android_library(name, **kwargs):
    native.filegroup(name = name)

def generate_jar_jni(name, **kwargs):
    native.filegroup(name = name)

def generate_jni(name, **kwargs):
    native.filegroup(name = name)

def generated_jni_library(name, **kwargs):
    native.filegroup(name = name)

def java_import(name, **kwargs):
    native.filegroup(name = name)

def rust_library(name, **kwargs):
    native.filegroup(name = name)
