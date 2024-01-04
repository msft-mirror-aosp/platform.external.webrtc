#!/usr/bin/env python
# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Exports the Google3 based webrtc build files into CMake files.


Google has a mechanism for converting Build.gn files into bazel files
which are used internally.

Bazel files are (simplified) python functions, and hence can be loaded
by the python interpreter.

We basically load a bazel file, and re-interpret the functions to create
an simplistic internal representation, which we turn into cmake files.
"""
import sys
import os.path
import argparse
import re
import platform
import logging
from glob import iglob
from datetime import date
from pathlib import Path


# The starting location of the webrtc modules.
PREFIX = "//third_party/webrtc/files/stable/webrtc"

# The prefix we use for every generated CMake library
WEBRTC_PREFIX = "webrtc_"

# Platform definitions, used by bazel to make selections on copt, defs etc.
# This mimics the effects of calling select:
# https://docs.bazel.build/versions/master/be/functions.html#select
PLATFORMS = {
    "Darwin": ["apple", "posix", "mac", "_all", "intel_cpu", ":cpu_x86_64"],
    "Darwin-aarch64": ["apple", "posix", "mac", "_all", "arm64", ":cpu_arm64"],
    "Linux-aarch64": [
        "linux_arm64",
        "linux",
        "posix",
        "linux_kernel",
        "arm64",
        "_all",
        ":cpu_arm64",
    ],
    "Linux": [
        "linux_x64",
        "linux",
        "posix",
        "linux_kernel",
        "_all",
        "intel_cpu",
        ":cpu_x86_64",
    ],
    "Windows": ["windows", "windows_x64", "intel_cpu", "_all", ":cpu_x86_64"],
}

# Default names of the generated cmake file for each target, your cmake script
# should include these files in order to compile and use the webrtc targets.
PLATFORM_NAME = {
    "Darwin": "darwin_x86_64.cmake",
    "Darwin-aarch64": "darwin_aarch64.cmake",
    "Linux-aarch64": "linux_aarch64.cmake",
    "Linux": "linux_x86_64.cmake",
    "Windows": "windows_x86_64.cmake",
}

# External dependencies that we translate to local ones. These are dependencies
# that live outside of the webrtc source tree and come from "external"
# libraries.
#
# The generated cmake files expect these dependencie to be available.
DEP_MAP = {
    ":AppRTCMobile_lib": "unknown",
    "//net/proto2/public:proto2_lite": "libprotobuf",
    "//testing/base/public:gunit_for_library_testonly": "gmock gtest",
    # Set of absl dependencies.
    "//third_party/absl/algorithm:container": "absl::algorithm_container",
    "//third_party/absl/strings:string_view" : "absl::string_view",
    "//third_party/absl/algorithm": "absl::algorithm",
    "//third_party/absl/base:config": "absl::config",
    "//third_party/absl/base:nullability": "absl::nullability",
    "//third_party/absl/base:core_headers": "absl::core_headers",
    "//third_party/absl/cleanup": "absl::cleanup",

    "//third_party/absl/container:flat_hash_map": "absl::flat_hash_map",
    "//third_party/absl/container:inlined_vector": "absl::algorithm_container",
    "//third_party/absl/debugging:failure_signal_handler": "absl::failure_signal_handler",
    "//third_party/absl/debugging:symbolize": "absl::debugging",
    "//third_party/absl/flags:config": "absl::flags_config",
    "//third_party/absl/flags:flag": "absl::flags",
    "//third_party/absl/flags:parse": "absl::flags_parse",
    "//third_party/absl/flags:usage": "absl::flags_usage",
    "//third_party/absl/functional:any_invocable": "absl::any_invocable",
    "//third_party/absl/functional:bind_front": "absl::bind_front",
    "//third_party/absl/memory": "absl::memory",
    "//third_party/absl/meta:type_traits": "absl::type_traits",
    "//third_party/absl/numeric:bits": "absl::bits",
    "//third_party/absl/strings": "absl::strings",
    "//third_party/absl/synchronization": "absl::synchronization",
    "//third_party/absl/types:optional": "absl::optional",
    "//third_party/absl/types:variant": "absl::variant",
    # The catapult dependencies.
    "//third_party/catapult/tracing:histogram": "webrtc_catapult_histogram",
    "//third_party/catapult/tracing:reserved_infos": "webrtc_catapult_reserved_infos",
    "//third_party/catapult/tracing/tracing/proto:histogram_proto_bridge": "webrtc_histogram_proto_bridge",
    "//third_party/crc32c": "crc32c",
    "//third_party/isac_fft:fft": "webrtc_fft",  # "unknown_fft",
    "//third_party/jsoncpp:json": "jsoncpp",
    "//third_party/libaom:aom_highbd": "webrtc_libaom",  #
    "//third_party/libevent": "libevent",
    "//third_party/libjpeg_turbo/src:jpeg": "emulator-libjpeg",
    "//third_party/libopus": "opus",
    "//third_party/libsrtp:srtp": "libsrtp",
    "//third_party/libyuv": "webrtc-yuv",
    "//third_party/objective_c/ocmock/v3:OCMock": "OCMock",
    "//third_party/ooura_fft:fft_size_128": "webrtc_fft_size_128",
    "//third_party/ooura_fft:fft_size_256": "webrtc_fft_size_256",
    "//third_party/openssl:crypto": "crypto",
    "//third_party/openssl:ssl": "ssl",
    "//third_party/pffft": "webrtc_pffft",
    "//third_party/dav1d:dav1d_lib": "dav1d",
    "//third_party/protobuf:protobuf-lite_legacy": "libprotobuf",
    "//third_party/protobuf:py_proto_runtime": "unknown_py_proto_runtime",
    "//third_party/rnnoise:rnn_vad": "webrtc_rnnoise",
    "//third_party/spl_sqrt_floor": "webrtc_spl_sqrt_floor",
    "//third_party/usrsctp": "usrsctp",
    "//third_party/webrtc:webrtc_libvpx": "libvpx",
    "//third_party/webrtc/files/override/webrtc/rtc_base:protobuf_utils": "libprotobuf",
    "//third_party/webrtc/files/testing:fileutils_override_google3": "emulator_test_overrides",
    "//third_party/webrtc/files/rtc_base:net_test_helpers": "",
    # We rely on Qt5 to provide all these..
    "//third_party/GL:GLX_headers": "Qt5::Core",
    "//third_party/Xorg:Xorg_static": "Qt5::Core",
    "//third_party/Xorg:libX11_static": "Qt5::Core",
    "//third_party/Xorg:libXcomposite_static": "Qt5::Core",
    "//third_party/Xorg:libXdamage_static": "Qt5::Core",
    "//third_party/Xorg:libXfixes_static": "Qt5::Core",
    "//third_party/Xorg:libXrandr_static": "Qt5::Core",
    "//third_party/Xorg:libXtst_static": "Qt5::Core",
    "//third_party/mesa:GL": "mesa",
    # Apple related frameworks
    "//third_party/apple_frameworks:Foundation": '"-framework Foundation"',
    "//third_party/apple_frameworks:ApplicationServices": '"-framework ApplicationServices"',
    "//third_party/apple_frameworks:AudioToolbox": '"-framework AudioToolbox"',
    "//third_party/apple_frameworks:CoreAudio": '"-framework CoreAudio"',
    "//third_party/apple_frameworks:CoreGraphics":'"-framework CoreGraphics"',
    # This is a genrule that we execute manually with the creation script, so we
    # can remove it.
    "//third_party/webrtc/files/stable/webrtc/experiments:registered_field_trials_header": "",
    # "//third_party/webrtc/files/stable/webrtc/api:field_trials_registry": "",
    # "//third_party/webrtc/files/stable/webrtc/experiments:registered_field_trials": "",
}

# Regexes of dependencies that can be safely ignored and do not need to be included anywhere
EXCLUDED_TARGETS = [
    # ".*dav1d.*",
    # ".*_unittests.*"
]

# Third party git dependencies, and the corresponding AOSP location
THIRD_PARTY_GIT_DEPS = {
    "src/third_party/jsoncpp/source": "jsoncpp",
    "src/third_party/libaom/source/libaom": "libaom",
    # "src/third_party/libjpeg_turbo",
    "src/third_party/libsrtp": "libsrtp2",
    "src/third_party/libvpx/source/libvpx": "libvpx",
    "src/third_party/libyuv": "libyuv",
    "src/third_party/usrsctp/usrsctplib": "usrsctp",
}

def exclude_target(name):
    """True if this target should be excluded when generating"""
    return any([re.match(regex, name) for regex in EXCLUDED_TARGETS])

def is_source_file(name):
    """Checks if a file is a source file.

    Args:
        name (str): The name of the file.

    Returns:
        bool: True if the file is a source file, False otherwise.
    """

    # Set of files we consider source
    SOURCE_EXT = [".c", ".cc", ".cpp", ".m", ".mm"]
    return any([name.endswith(x) for x in SOURCE_EXT])


def bazel_package_name(target):
    """Returns the bazel package name.

    Args:
        target (str): The target name.

    Returns:
        str: The bazel package name.
    """
    idx = target.find(":")

    # If there is no colon, the target is the package name
    if idx < 0:
        return target

    # Otherwise, the package name is the substring before the first colon
    return target[:idx]


def bazel_label_name(target):
    """Gets the label from the complete bazel target.

    The name of a target is called its label. Every label uniquely identifies a
    target.

    Args:
        target (str): The target name.

    Returns:
        str: The label name.
    """
    label = target[target.rfind("/") + 1 :]
    return label[label.find(":") + 1 :]


def cmake_target_to_bazel_target(path, label):
    """Translates the cmake name back to a bazel target.

    Args:
        path (str): The path to the target.
        label (str): The label of the target.

    Returns:
        str: The bazel target name.
    """

    # If the path is the same as the label, the bazel target name is the path
    if os.path.basename(path) == label:
        return "{}/{}".format(PREFIX, path)

    # Otherwise, the bazel target name is the path followed by a colon and the label
    return "{}/{}:{}".format(PREFIX, path, label)


def bazel_target_to_cmake_target(name):
    """
    Translates a bazel target to a cmake target.

    Translation happens as follows:

    1. Return the default mapping if defined.
    2. Translate the name to webrtc_dir_of_package_label

    Args:
        name (str): The name of the target.

    Returns:
        str: The cmake target name.
    """

    # If the target is in the default mapping, return the default mapping
    if name in DEP_MAP:
        return DEP_MAP[name]

    # Get the package and label of the target
    pkg = bazel_package_name(name)
    lbl = bazel_label_name(name)

    if not pkg.startswith(PREFIX):
        logging.fatal("Missing library: %s", name)
        return "missing_{}".format(name).replace("/", "_")

    # Get the cmake target name
    target = "{}{}_{}".format(WEBRTC_PREFIX, pkg[len(PREFIX) + 1 :], lbl).replace(
        "/", "_"
    )

    return target


def clang_opts_fixer(copts):
    """Fixes clang options, by doing variable substition and / to -/.

    This contains a set of clang specific translations that are needed to compile
    using the
    emulator compile toolchain.

    Args:
        copts (list): The clang options.

    Returns:
        list: The fixed clang options.
    """
    flag_replace = {
        "/GR-": "-GR-",  # We are using clang, not msvc.
        "/arch:AVX2": ("-mavx2 -mfma"),  # We are using clang specification over cl's
        "/TC": "-TC",
        "-Wctad-maybe-unsupported": "", # We use gcc for arm, and it cannot handle this.
        "$(WEBRTC_RTTI)": "",  # No exceptions already.
    }
    return [flag_replace[x] if x in flag_replace else x for x in copts]


class Target(object):
    """Represents a bazel target."""

    def __init__(self, path, name, typ, srcs, hdrs, defs, deps, copts, includes, lopts):
        self.path = path
        self.label = name
        self.name = "{}{}_{}".format(WEBRTC_PREFIX, path, name).replace("/", "_")
        self.typ = typ
        self.srcs = srcs
        self.hdrs = hdrs
        self.defs = defs
        self.deps = [x for x in deps if not exclude_target(x)]
        self.includes = [
            "${{WEBRTC_ROOT}}/{}/{}".format(self.path, x) for x in includes
        ] + ["${WEBRTC_ROOT}", "${CMAKE_CURRENT_BINARY_DIR}"]
        # TODO variable replacement should be done for all.
        self.copts = clang_opts_fixer(copts)
        self.lopts = [
            "{lib}::{lib}".format(lib=x[:-4]) for x in lopts if x.endswith(".lib")
        ]

    def _add_deps(self, name, deps, sort="PUBLIC"):
        common = [bazel_target_to_cmake_target(x) for x in deps]
        return "target_link_libraries({} {} {} {})\n".format(
            name, sort, " ".join(common), " ".join(self.lopts)
        )

    def is_interface(self):
        files = self.srcs + self.hdrs
        return not any([is_source_file(x) for x in files])

    def cmake_snippet(self):
        """Translate the bazel target into a cmake snippet."""
        snippet = "# {}\n".format(
            cmake_target_to_bazel_target(self.path, self.label)[len(PREFIX) + 1 :]
        )

        if self.is_interface() and self.typ != "add_executable":
            snippet += "add_library({} INTERFACE)\n".format(self.name)
            if self.deps:
                snippet += self._add_deps(self.name, self.deps, "INTERFACE")
            snippet += "target_include_directories({} INTERFACE {})\n".format(
                self.name, " ".join(self.includes)
            )
            return snippet

        sources = ["${{WEBRTC_ROOT}}/{}/{}".format(self.path, x) for x in self.srcs]

        if self.is_interface():
            sources = ["${CMAKE_CURRENT_BINARY_DIR}/dummy.cc"]

        if self.typ == "android_add_test":
            snippet += (
                "  android_add_executable(TARGET {} NODISTRIBUTE SRC {})\n".format(
                    self.name, " ".join(sources)
                )
            )
        else:
            snippet += "{}({} {})\n".format(self.typ, self.name, " ".join(sources))
        snippet += "target_include_directories({} PRIVATE {})\n".format(
            self.name, " ".join(self.includes)
        )

        if self.defs:
            snippet += "target_compile_definitions({} PRIVATE {})\n".format(
                self.name, " ".join(self.defs)
            )

        if self.copts:
            snippet += "target_compile_options({} PRIVATE {})\n".format(
                self.name, " ".join(self.copts)
            )

        if self.deps:
            snippet += self._add_deps(self.name, self.deps)

        return snippet


class ObjcTarget(Target):
    """An apple specific objective-c target.

    An objective-c target can have a set of sdk (macos framework) dependencies.
    """

    def __init__(self, path, name, typ, srcs, hdrs, defs, deps, copts, includes, sdk):
        super(ObjcTarget, self).__init__(
            path, name, typ, srcs, hdrs, defs, deps, copts, includes, []
        )
        self.sdk = sdk

    def cmake_snippet(self):
        snippet = super(ObjcTarget, self).cmake_snippet()
        sdks = ['"-framework {}"'.format(x) for x in self.sdk]
        sort = "INTERFACE" if self.is_interface() else "PRIVATE"
        snippet += "target_link_libraries({} {} {})\n".format(
            self.name, sort, " ".join(sdks)
        )
        return snippet


class ProtobufTarget(object):
    """A Protobuf target is a collection of .proto files that need to be compiled."""

    def __init__(self, path, name, srcs, deps, alias=None):
        self.path = path
        self.label = name
        self.internal_name = "{}{}_{}".format(WEBRTC_PREFIX, path, name).replace(
            "/", "_"
        )
        self.name = self.internal_name
        self.srcs = srcs
        self.deps =  [x for x in deps if not exclude_target(x)]
        self.alias = alias

    def cmake_snippet(self):
        snippet = "# {}\n".format(cmake_target_to_bazel_target(self.path, self.label))
        snippet += "add_library({})\n".format(self.name)

        src_snippet = ""
        path_snippet = ""
        dest_path = ""
        for src in self.srcs:
            # Hack attack! We assume that all protobufs are from the same
            # dir..
            dest_path = os.path.dirname(os.path.join(self.path, src))
            src_snippet += " ${{WEBRTC_ROOT}}/{}/{}".format(self.path, src)
            path_snippet += " -I${{WEBRTC_ROOT}}/{}".format(dest_path)

        snippet += """protobuf_generate_with_plugin(
  TARGET {tgt}
  PROTOS {src}
  HEADERFILEEXTENSION .pb.h
  APPEND_PATH
  PROTOPATH {include_path}
  PROTOC_OUT_DIR ${{CMAKE_CURRENT_BINARY_DIR}}/{path})\n""".format(
            tgt=self.name, path=dest_path, include_path=path_snippet, src=src_snippet
        )
        snippet += "target_include_directories({} PUBLIC  ${{CMAKE_CURRENT_BINARY_DIR}}/{})\n".format(
            self.name, dest_path
        )
        snippet += "add_library({}_lib ALIAS {})\n".format(
            self.internal_name, self.name
        )
        snippet += "target_link_libraries({} PUBLIC libprotobuf)\n".format(self.name)
        if self.deps:
            common = [bazel_target_to_cmake_target(x) for x in self.deps]
            snippet += "target_link_libraries({} PRIVATE {})\n".format(
                self.name, " ".join(common)
            )

        if self.alias:
            for a in self.alias:
                snippet += "add_library({}_lib ALIAS {})\n".format(a, self.name)

        return snippet


class Targets(object):
    """A collection of targets."""

    def __init__(self):
        self.targets = {}

    def add(self, tgt):
        logging.debug("Adding target: %s", tgt.name)
        self.targets[tgt.name] = tgt

    def _calc_closure(self, name, seen):
        if name not in self.targets:
            return set()

        tgt = self.targets[name]
        if tgt in seen:
            return set()

        seen.add(tgt)
        for dep in tgt.deps:
            # Normalize the dependency
            seen |= self._calc_closure(bazel_target_to_cmake_target(dep), seen)

        return seen

    def closure(self, name):
        """Returns the target closure for the target of the given name.

        This calculates all the dependencies needed to compile the given target."""
        return self._calc_closure(name, set())

    def platform_definition_snippet(self):
        """Writes out the platform target definitions, extracted from webrtc_lib_link_test.

        This is a workaround to extract all the required -D definitions that are needed to successfully
        take a dependency on any webrtc target. Definitions like
        -DWEBRTC_POSIX/-DWEBRTC_WIN will end up here.

        Without these definitions you will not be able to successfully include a webrtc header.
        """
        target = self.targets["webrtc__webrtc_lib_link_test"]
        snippet = "# Platform specific set of defines\n"
        snippet += "add_library(webrtc_platform_defs INTERFACE)\n"
        snippet += (
            "target_compile_definitions(webrtc_platform_defs INTERFACE {}\n)".format(
                " ".join(target.defs)
            )
        )
        return snippet


class BuildFileFunctions(object):
    """This class is responsible for the actual bazel -> Target compilation."""

    def __init__(self, targets, fname, start_dir, platforms):
        self.targets = targets
        self.start_dir = os.path.abspath(start_dir)
        self.rel = os.path.relpath(
            os.path.dirname(os.path.abspath(fname)),
            self.start_dir,
        )
        if self.rel == ".":
            self.rel = ""
        self.platforms = platforms

    def _add_target(self, typ, name, srcs, hdrs, defs, deps, copts, lopts, includes):
        # HACK ATTACK b/191745658, the neon build has a dependency at the wrong
        # level, so we correct it here, this should be removed once the fix is
        # in.
        if exclude_target(name):
            logging.info("Ignoring %s, it's in the excluded list",)
            return

        if name == "common_audio_c" and "arm64" in self.platforms:
            logging.error("Fixing up common_audio_c depedency b/191745658!")
            if (
                "//third_party/webrtc/files/stable/webrtc/common_audio:common_audio_neon"
                in deps
            ):
                logging.fatal("The workaround is no longer needed! Please remove it!")
            deps.append(
                "//third_party/webrtc/files/stable/webrtc/common_audio:common_audio_neon"
            )

        self.targets.add(
            Target(self.rel, name, typ, srcs, hdrs, defs, deps, copts, includes, lopts)
        )

    def load(self, *args):
        pass

    def py_strict_binary(self, **kwargs):
        pass

    def android_jni_library(self, **kwargs):
        pass

    def ios_application(self, **kwargs):
        pass

    def alias(self, **kwargs):
        pass

    def ios_unit_test(self, **kwargs):
        pass

    def android_instrumentation_test(self, **kwargs):
        pass

    def java_cpp_enum(self, **args):
        pass

    def swift_interop_hint(self, **args):
        pass

    def libaom_encoder_dependency(self, **args):
        return ["//third_party/libaom:aom_highbd"]

    def libvpx_dependency(self, **args):
        return ["//third_party/webrtc:webrtc_libvpx"]

    def setup_webrtc_build(self, *args):
        pass

    def android_library(self, **kwargs):
        pass

    def go_proto_library(self, **kwargs):
        pass

    def filegroup(self, **kwargs):
        pass

    def platform_select(self, **kwargs):
        selected = []
        for plf in self.platforms:
            selected += kwargs.get(plf, [])
        return selected

    def generated_jar_jni_library(self, **kwargs):
        pass

    def select(self, **kwargs):
        selected = []
        for plf in self.platforms:
            selected += kwargs.get(plf, [])
        return selected

    def generated_jni_library(self, **kwargs):
        pass

    def android_binary(self, **kwargs):
        pass

    def java_import(self, **kwargs):
        pass

    def webrtc_grpc_library(self, **kwargs):
        # TODO Figure out what goes on here
        self.targets.add(
            ProtobufTarget(
                self.rel,
                "%s_cc_proto" % kwargs["name"],
                kwargs["srcs"],
                kwargs.get("deps", []),
            )
        )

    def webrtc_proto_library(self, **kwargs):
        self.targets.add(
            ProtobufTarget(
                self.rel,
                kwargs["name"],
                kwargs["srcs"],
                kwargs.get("deps", []),
            )
        )

    def cc_library(self, **kwargs):
        self._add_target(
            "add_library",
            kwargs["name"],
            kwargs.get("srcs", []),
            kwargs.get("hdrs", []),
            kwargs.get("defines", []),
            kwargs.get("deps", []),
            kwargs.get("copts", []),
            kwargs.get("linkopts", []),
            kwargs.get("includes", []),
        )

    def objc_library(self, **kwargs):
        if "apple" in self.platforms:
            self.targets.add(
                ObjcTarget(
                    self.rel,
                    kwargs["name"],
                    "add_library",
                    kwargs.get("srcs", []),
                    kwargs.get("hdrs", []),
                    kwargs.get("defines", []),
                    kwargs.get("deps", []),
                    kwargs.get("copts", []) + ["-fobjc-weak"],
                    kwargs.get("includes", []),
                    kwargs.get("sdk_frameworks", []),
                )
            )

    def cc_binary(self, **kwargs):
        self._add_target(
            "add_executable",
            kwargs["name"],
            kwargs.get("srcs", []),
            kwargs.get("hdrs", []),
            kwargs.get("defines", []),
            kwargs.get("deps", []),
            kwargs.get("copts", []),
            kwargs.get("linkopts", []),
            kwargs.get("includes", []),
        )

    def cc_test(self, **kwargs):
        self._add_target(
            "add_executable",
            kwargs["name"],
            kwargs.get("srcs", []),
            kwargs.get("hdrs", []),
            kwargs.get("defines", []),
            kwargs.get("deps", []),
            kwargs.get("copts", []),
            kwargs.get("linkopts", []),
            kwargs.get("includes", []),
        )

    def cc_and_android_test(self, **kwargs):
        self._add_target(
            "android_add_test",
            kwargs["name"],
            kwargs.get("srcs", []),
            kwargs.get("hdrs", []),
            kwargs.get("defines", []),
            kwargs.get("deps", []),
            kwargs.get("copts", []),
            kwargs.get("linkopts", []),
            kwargs.get("includes", []),
        )

    def build_test(self, **kwargs):
        pass

    def py_library(self, **kwargs):
        pass

    def py_binary(self, **kwargs):
        pass

    def package(self, **kwargs):
        pass

    def sh_test(self, **kwargs):
        pass

    def make_shell_script(self, **kwargs):
        pass

    def exports_files(self, files, **kwargs):
        pass

    def proto_library(self, **kwargs):
        self.targets.add(ProtobufTarget(self.rel, kwargs["name"], kwargs["srcs"]))

    def generated_file_staleness_test(self, **kwargs):
        pass

    def upb_proto_library(self, **kwargs):
        pass

    def jspb_proto_library(self, **kwargs):
        pass

    def java_proto_library(self, **kwargs):
        pass

    def swift_proto_library(self, **kwargs):
        pass

    def upb_proto_reflection_library(self, **kwargs):
        pass

    def genrule(self, **kwargs):
        pass

    def config_setting(self, **kwargs):
        pass

    def select(self, arg_dict):
        return []

    def glob(self, *args):
        return []

    def licenses(self, *args):
        pass

    def map_dep(self, arg):
        return arg


def write_cmake_project(targets, dependencies, platform):
    cmake = "# Generated on {} for target: {}\n".format(
        date.today().strftime("%m/%d/%y"), platform
    )
    cmake += "# This is an autogenerated file by calling:\n\n"
    cmake += "# {}\n\n".format(" ".join(sys.argv))
    cmake += "# Re-running this script will require you to merge in the latest upstream-master for webrtc\n\n"
    for aosp, sha in dependencies.items():
        cmake += "# Expecting {}  at {}\n".format(aosp, sha)
    cmake += """\n\n
# Create a symlink so webrtc can find required includes, we already have all
# the required dependencies in our tree.
message("Creating webrtc test symlinks ${CMAKE_CURRENT_BINARY_DIR}/testing/")
android_symlink("${AOSP_ROOT}/external/googletest/googletest/" "gtest" ${CMAKE_CURRENT_BINARY_DIR}/testing)
android_symlink("${AOSP_ROOT}/external/googletest/googlemock/" "gmock" ${CMAKE_CURRENT_BINARY_DIR}/testing)

# ==== Generated modules are coming next ====
    """

    # Sort targets, lest we get non-deterministic behavior.
    sorted_targets = sorted(targets, key=lambda target: target.name)
    for target in sorted_targets:
        cmake += "\n" + target.cmake_snippet()

    return cmake


def GetDict(obj):
    ret = {}
    for k in dir(obj):
        if not k.startswith("_"):
            ret[k] = getattr(obj, k)
    return ret


def transform_bazel(bld_file, root_dir, platform, converter):
    logging.info("  %s - %s", platform, bld_file)
    exec(
        compile(open(bld_file, "rb").read(), bld_file, "exec"),
        GetDict(BuildFileFunctions(converter, bld_file, root_dir, PLATFORMS[platform])),
    )


def extract_dependencies(deps_file):
    """Extracts the dependency dictionary from the given deps_file.

    Returns a map with { aosp_location -> git_sha }.
    """
    dependencies = {}
    exec(
        compile(
            "def Var(x): return vars[x]\n\n" + open(deps_file, "r").read(),
            deps_file,
            "exec",
        ),
        dependencies,
    )
    deps = dependencies.get("deps", {})
    local_deps = {}
    for dep in THIRD_PARTY_GIT_DEPS.keys():
        loc = THIRD_PARTY_GIT_DEPS[dep]
        url = deps.get(dep, {})
        if "@" in url:
            local_deps[loc] = url.split("@")[1]
    return local_deps


def main():
    if sys.version_info[0] == 2:
        print("This only runs under Python3")
        sys.exit(1)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--verbose", help="increase output verbosity", action="store_true"
    )
    parser.add_argument(
        "--platform",
        default=platform.system(),
        choices=(PLATFORMS.keys()),
        help="Target platform to generate cmake file for.",
    )
    parser.add_argument(
        "--root",
        default=os.path.abspath(os.getcwd()),
        help="Root directory to map the BUILD root to.",
    )
    parser.add_argument(
        "--deps",
        default="DEPS",
        help="Dependency file, used to extract git revisions for dependencies.",
    )

    parser.add_argument(
        "--target",
        action="append",
        help="Target closure to extract.",
    )

    parser.add_argument(
        "source",
        default="BUILD",
        help="First BUILD file to process, will descent recursively to load all of them.",
    )

    parser.add_argument(
        "out",
        default=None,
        help="Dir, or cmake file to write to.",
    )

    args = parser.parse_args()

    # Configure logger.
    lvl = logging.DEBUG if args.verbose else logging.WARNING
    handler = logging.StreamHandler()
    logging.root = logging.getLogger("root")
    logging.root.addHandler(handler)
    logging.root.setLevel(lvl)

    # Extract the current dependencies.
    dep_path = Path(args.root, args.deps)
    if dep_path.exists():
        dependencies = extract_dependencies(os.path.join(args.root, args.deps))
    targets = Targets()

    source = os.path.join(args.root, args.source)
    transform_bazel(source, args.root, args.platform, targets)
    start_dir = os.path.dirname(source)
    for fn in iglob(start_dir + "/**/BUILD", recursive=True):
        if os.path.isfile(fn):
            transform_bazel(fn, args.root, args.platform, targets)

    cmake = set()
    for x in args.target:
        cmake |= targets.closure(x)

    out = sys.stdout

    if args.out:
        if os.path.isdir(args.out):
            out = open(os.path.join(args.out, PLATFORM_NAME[args.platform]), "w")
        else:
            out = open(args.out, "w")

    out.write(write_cmake_project(cmake, dependencies, args.platform))
    out.write(targets.platform_definition_snippet())


if __name__ == "__main__":
    main()
