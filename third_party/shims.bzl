load("@rules_cc//cc:defs.bzl", "cc_library")

def _generate_forwarding_headers_impl(ctx):
    outputs = []
    for h in ctx.attr.headers:
        out_file = ctx.actions.declare_file(ctx.attr.prefix + "/" + h)
        outputs.append(out_file)

        content = "#pragma once\n#include <{header}>\n".format(header = h)

        ctx.actions.write(
            output = out_file,
            content = content,
        )
    return [DefaultInfo(files = depset(outputs))]

generate_forwarding_headers = rule(
    implementation = _generate_forwarding_headers_impl,
    attrs = {
        "headers": attr.string_list(mandatory = True),
        "prefix": attr.string(default = "shim"),
    },
)

def forwarding_shim(name, headers, backend, include_prefix, visibility = None):
    gen_target = name + "_gen_headers"

    generate_forwarding_headers(
        name = gen_target,
        headers = headers,
        prefix = "shim",
    )

    cc_library(
        name = name,
        hdrs = [":" + gen_target],
        strip_include_prefix = "shim",
        include_prefix = include_prefix,
        deps = [backend],
        visibility = visibility,
    )
