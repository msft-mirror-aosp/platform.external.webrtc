# load("@bazel_tools//tools/genrule/genrule.bzl", "genrule")

def generate_resource_genrules(name, resource_files, script = "//resources/bazel:resource_downloader"):
    for resource in resource_files:
        sha1_file = str(resource)
        if sha1_file.endswith(".sha1"):
            yuv_file = sha1_file[:-5]  # Remove the .sha1 extension
            native.genrule(
                name = name + "_" + yuv_file.replace("/", "_").replace(".", "_"),
                srcs = [sha1_file],
                outs = [yuv_file],
                cmd = "$(location {script}) $(location {sha1}) $(location {yuv})".format(
                    script = script,
                    sha1 = sha1_file,
                    yuv = yuv_file,
                ),
                tools = [script],
                visibility = ["//visibility:public"],
            )
