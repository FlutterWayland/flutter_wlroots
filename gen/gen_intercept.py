def lookahead(iterable):
    """Pass through all values from the given iterable, augmented by the
    information if there are more values to come after the current one
    (True), or if it is the last value (False).
    """
    # Get an iterator and pull the first value.
    it = iter(iterable)
    try:
        last = next(it)
    except StopIteration:
        return
    # Run the iterator to exhaustion (starting from the second value).
    for val in it:
        # Report the *previous* value (more to come).
        yield last, True
        last = val
    # Report the last value.
    yield last, False

#from bs4 import BeautifulSoup
from xml.etree import ElementTree as ET

data = ET.parse("gl.xml")

root = data.getroot()

commands_container = root.find("commands")
commands = commands_container.findall("command")

enums = root.findall("enums")

out_commands = []

for command in commands:
    out = {}

    proto = command.find("proto")
    proto_name = proto.find("name").text
    out["name"] = proto_name

    proto.remove(proto.find("name"))
    proto_type = ET.tostring(proto, method="text").decode("utf8").strip()
    #proto_type = proto.find("ptype")
    #if proto_type is None:
    #    proto_type = proto.text.strip()
    #else:
    #    proto_type = proto_type.text
    out["rtype"] = proto_type

    out_params = []
    params = command.findall("param")
    for param in params:
        out_param = {}
        attrib = param.attrib
        if "class" in attrib:
            out_param["class"] = attrib["class"]
        if "group" in attrib:
            out_param["group"] = attrib["group"]
        out_param["name"] = param.find("name").text
        param.remove(param.find("name"))
        ptype = ET.tostring(param, method="text").decode("utf8").strip()
        out_param["ptype"] = ptype
        out_params.append(out_param)

    out["params"] = out_params

    out_commands.append(out)

defined_enums = set()
out_enums = []
for enum in enums:
    enum_attrs = enum.attrib
    if "group" not in enum_attrs and enum_attrs.get("namespace") == "GL" and enum_attrs.get("vendor") == "ARB":
        for enum in enum.findall("enum"):
            if enum.attrib["value"] in defined_enums:
                print("WARNING: duplicate enum " + enum.attrib["value"] + " " + enum.attrib["name"])
            else:
                out_enums.append((enum.attrib["value"], enum.attrib["name"]))
                defined_enums.add(enum.attrib["value"])
#print(out_enums)

formatters = {
    "const GLubyte *": "%s",
}
default_formatter = "%ld"

o = ""
#o += "#include \"dummy_head.h\"\n"
o += "#include <GLES2/gl2.h>\n"
o += "#include \"gl_intercept_dummy.h\"\n"
o += "#include <string.h>\n\n"

o += "char *__gl_intercept_formatenum(GLenum val) {\n"
o += "  switch (val) {\n"
for (val, name) in out_enums:
    o += "  case " + val + ":\n"
    o += "    return \"" + name + "\";\n"
o += "  default:\n"
o += "    return \"ENUM_UNKNOWN\";\n"
o += "  }\n"
o += "}\n\n"

for command in out_commands:
    is_void_ret = command["rtype"] == "void"

    ctype = "typedef " + command["rtype"] + "(*__ctype_" + command["name"] + ")("
    for param, has_more in lookahead(command["params"]):
        ctype += param["ptype"]
        if has_more:
            ctype += ", "
    ctype += ");\n"

    fnvar = "__glintercept_fnptr_" + command["name"]

    o += ctype
    o += "static __ctype_" + command["name"] + " " + fnvar + ";\n"

    o += command["rtype"] + " __glintercept_" + command["name"] + "("
    for param, has_more in lookahead(command["params"]):
        o += param["ptype"] + " " + param["name"]
        if has_more:
            o += ", "
    o += ") {\n"

    #o += "  if (" + fnvar + " == NULL) {\n"
    #o += "    " + fnvar + " = (__ctype_" + command["name"] + ") eglGetProcAddr(\"" + command["name"] + "\");\n"
    #o += "  }\n"

    if is_void_ret: 
        o += "  "
    else:
        o += "  " + command["rtype"] + " __ret = "
    o += fnvar + "("
    for param, has_more in lookahead(command["params"]):
        o += param["name"]
        if has_more:
            o += ", "
    o += ");\n"

    o += "  __glintercept_log(\" - " + command["name"] + "("
    for param, has_more in lookahead(command["params"]):
        formatter = formatters.get(param["ptype"], default_formatter)
        o += param["name"] + ": " + formatter
        if param["ptype"] == "GLenum":
            o += " (%s)"
        if has_more:
            o += ", "
    o += ")"
    if not is_void_ret:
        formatter = formatters.get(command["rtype"], default_formatter)
        o += " -> " + formatter
    o += "\"";
    for param in command["params"]:
        o += ", " + param["name"]
        if param["ptype"] == "GLenum":
            o += ", __gl_intercept_formatenum(" + param["name"] + ")"
    if not is_void_ret:
        o += ", __ret"
    o += ");\n"

    if not is_void_ret:
        o += "  return __ret;\n"

    o += "}\n\n"

o += "\n"

o += "void *glintercept_getProcAddress(const char* name) {\n"
for command in out_commands:
    fnvar = "__glintercept_fnptr_" + command["name"]
    o += "  if (strcmp(name, \"" + command["name"] + "\") == 0) {\n"
    o += "    if (" + fnvar + " == NULL) {\n"
    o += "      " + fnvar + " = (__ctype_" + command["name"] + ") eglGetProcAddr(\"" + command["name"] + "\");\n"
    o += "    }\n"
    o += "    if (" + fnvar + " == NULL) return NULL;\n"
    o += "    return (void*) __glintercept_" + command["name"] + ";\n"
    o += "  }\n"
o += "  return (void*) 0;\n"
o += "}\n"

with open("../include/gl_intercept_debug.h", "w") as f:
    f.write(o)
