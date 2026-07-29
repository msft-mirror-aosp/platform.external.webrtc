load("@rules_python//python:defs.bzl", _py_binary = "py_binary")

def py_binary(name, **kwargs):
    kwargs.pop("launcher", None)
    kwargs.pop("strict_deps", None)
    _py_binary(name = name, **kwargs)
