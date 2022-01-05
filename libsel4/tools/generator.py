#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

from functools import reduce
import itertools
import operator
import xml.dom.minidom


# Maximum number of words that will be in a message.
MAX_MESSAGE_LENGTH = 64


class Parameter(object):
    def __init__(self, name, type):
        self.name = name
        self.type = type


class Api(object):
    def __init__(self, node):
        self.name = node.getAttribute("name")
        self.label_prefix = node.getAttribute("label_prefix") or ""


def is_result_struct_required(output_params) -> bool:
    return len([x for x in output_params if not x.type.pass_by_reference()]) != 0


def struct_members(typ, structs):
    members = [member for struct_name, member in structs if struct_name == typ.name]
    assert len(members) == 1
    return members[0]


# Keep increasing the given number 'x' until 'x % a == 0'.
def align_up(x, a):
    if x % a == 0:
        return x
    return x + a - (x % a)


def normalise_text(text):
    """
    Removes leading and trailing whitespace from each line of text.
    Removes leading and trailing blank lines from text.
    """
    stripped_lines = [line.strip() for line in text.split("\n")]
    # remove leading and trailing empty lines
    stripped_head = list(itertools.dropwhile(lambda s: not s, stripped_lines))
    stripped_tail = itertools.dropwhile(lambda s: not s, reversed(stripped_head))
    return "\n".join(reversed(list(stripped_tail)))


def get_parameter_positions(parameters, wordsize):
    """
    Determine where each parameter should be packed in the generated message.
    We generate a list of:

        (param_name, param_type, first_bit, num_bits)

    tuples.

    We guarantee that either (num_words == 1) or (bit_offset == 0).
    """
    bits_used = 0
    results = []

    for param in parameters:
        # How big are we?
        type_size = param.type.size_bits

        # We need everything to be a power of two, or word sized.
        assert ((type_size & (type_size - 1)) == 0) or (type_size % wordsize == 0)

        # Align up to our own size, or the next word. (Whichever is smaller)
        bits_used = align_up(bits_used, min(type_size, wordsize))

        # Place ourself.
        results.append((param, bits_used, type_size))
        bits_used += type_size

    return results


def get_xml_element_content_with_xmlonly(element):
    """
    Converts the contents of an xml element into a string, wrapping
    all child xml nodes in doxygen @xmlonly/@endxmlonly keywords.
    """

    result = []
    prev_element = False
    for node in element.childNodes:
        if node.nodeType == xml.dom.Node.TEXT_NODE:
            if prev_element:
                # text node following element node
                result.append(" @endxmlonly ")
            prev_element = False
        else:
            if not prev_element:
                # element node following text node
                result.append(" @xmlonly ")
            prev_element = True

        result.append(node.toxml())

    return "".join(result)


def get_xml_element_contents(element):
    """
    Converts the contents of an xml element into a string, with all
    child xml nodes unchanged.
    """
    return "".join([c.toxml() for c in element.childNodes])


def parse_xml_file(input_file, valid_types):
    """
    Parse an XML file containing method definitions.
    """

    # Create a dictionary of type name to type.
    type_names = {}
    for i in valid_types:
        type_names[i.name] = i

    # Parse the XML to generate method structures.
    methods = []
    structs = []
    doc = xml.dom.minidom.parse(input_file)

    api = Api(doc.getElementsByTagName("api")[0])

    for struct in doc.getElementsByTagName("struct"):
        _struct_members = []
        struct_name = struct.getAttribute("name")
        for members in struct.getElementsByTagName("member"):
            member_name = members.getAttribute("name")
            _struct_members.append(member_name)
        structs.append((struct_name, _struct_members))

    for interface in doc.getElementsByTagName("interface"):
        interface_name = interface.getAttribute("name")
        interface_manual_name = interface.getAttribute("manual_name") or interface_name

        interface_cap_description = interface.getAttribute("cap_description")

        for method in interface.getElementsByTagName("method"):
            method_name = method.getAttribute("name")
            method_id = method.getAttribute("id")
            method_condition = method.getAttribute("condition")
            method_manual_name = method.getAttribute("manual_name") or method_name
            method_manual_label = method.getAttribute("manual_label")

            if not method_manual_label:
                # If no manual label is specified, infer one from the interface and method
                # names by combining the interface name and method name.
                method_manual_label = ("%s_%s" % (interface_manual_name, method_manual_name)) \
                    .lower() \
                    .replace(" ", "_") \
                    .replace("/", "")

            # Prefix the label with an api-wide label prefix
            method_manual_label = "%s%s" % (api.label_prefix, method_manual_label)

            comment_lines = ["@xmlonly <manual name=\"%s\" label=\"%s\"/> @endxmlonly" %
                             (method_manual_name, method_manual_label)]

            method_brief = method.getElementsByTagName("brief")
            if method_brief:
                method_brief_text = get_xml_element_contents(method_brief[0])
                normalised_method_brief_text = normalise_text(method_brief_text)
                comment_lines.append("@brief @xmlonly %s @endxmlonly" %
                                     normalised_method_brief_text)

            method_description = method.getElementsByTagName("description")
            if method_description:
                method_description_text = get_xml_element_contents(method_description[0])
                normalised_method_description_text = normalise_text(method_description_text)
                comment_lines.append("\n@xmlonly\n%s\n@endxmlonly\n" %
                                     normalised_method_description_text)

            #
            # Get parameters.
            #
            # We always have an implicit cap parameter.
            #
            input_params = [Parameter("_service", type_names[interface_name])]

            cap_description = interface_cap_description
            cap_param = method.getElementsByTagName("cap_param")
            if cap_param:
                append_description = cap_param[0].getAttribute("append_description")
                if append_description:
                    cap_description += append_description

            comment_lines.append("@param[in] _service %s" % cap_description)
            output_params = []
            for param in method.getElementsByTagName("param"):
                param_name = param.getAttribute("name")
                param_type = type_names.get(param.getAttribute("type"))
                if not param_type:
                    raise Exception("Unknown type '%s'." % (param.getAttribute("type")))
                param_dir = param.getAttribute("dir")
                assert (param_dir == "in") or (param_dir == "out")
                if param_dir == "in":
                    input_params.append(Parameter(param_name, param_type))
                else:
                    output_params.append(Parameter(param_name, param_type))

                if param_dir == "in" or param_type.pass_by_reference():
                    param_description = param.getAttribute("description")
                    if not param_description:
                        param_description_element = param.getElementsByTagName("description")
                        if param_description_element:
                            param_description_text = get_xml_element_content_with_xmlonly(
                                param_description_element[0])
                            param_description = normalise_text(param_description_text)

                    comment_lines.append("@param[%s] %s %s " %
                                         (param_dir, param_name, param_description))

            method_return_description = method.getElementsByTagName("return")
            if method_return_description:
                comment_lines.append("@return @xmlonly %s @endxmlonly" %
                                     get_xml_element_contents(method_return_description[0]))
            else:
                # no return documentation given - default to something sane
                if is_result_struct_required(output_params):
                    comment_lines.append("@return @xmlonly @endxmlonly")
                else:
                    comment_lines.append("@return @xmlonly <errorenumdesc/> @endxmlonly")

            for error in method.getElementsByTagName("error"):
                error_name = error.getAttribute("name")
                error_description = error.getAttribute("description")
                if not error_description:
                    error_description_element = error.getElementsByTagName("description")
                    if error_description_element:
                        error_description_text = get_xml_element_content_with_xmlonly(
                            error_description_element[0])
                        error_description = normalise_text(error_description_text)
                comment_lines.append("@retval %s %s " % (error_name, error_description))

            # split each line on newlines
            comment_lines = reduce(operator.add, [l.split("\n") for l in comment_lines], [])

            methods.append((interface_name, method_name, method_id, input_params,
                            output_params, method_condition, comment_lines))

    return (methods, structs, api)


class Generator:
    '''
    All the _gen_* functions are used to generate some piece of code.
    The generated code is appended to self.contents which is a list of lines
    '''

    def _gen_call(self, service_cap, num_mrs):
        '''
        Generate call using a service capability
        '''
        raise NotImplementedError()

    def _gen_comment(self, lines, *, doc=False, indent=0, inline=False):
        """
        Generate a multiline comment string with the content of 'lines'
        'lines' is an iterable of strings.
        Each string will be placed in a new line in the comment
        'doc' allows changing the format to documentation comment
        """
        raise NotImplementedError()

    def _gen_conditional_compilation(self, condition) -> str:
        '''
        Generate preprocessor directives to compile code conditionally
        For languages which have 'begin <func> end' syntax,
        this function generates the `begin` and returns the `end`
        For C, it generates #ifdef, and returns #endif.

        TODO currently 'condition' is specific to the C language.
        '''
        raise NotImplementedError()

    def _gen_declare_variables(self, returning_struct: bool, return_type, method_id, cap_expressions, input_expressions):
        '''
        Generate code with variable declaration for return and temporary values
        '''
        raise NotImplementedError()

    def _gen_file_footer(self):
        '''
        Generates things such as #endif for global guards
        '''
        raise NotImplementedError()

    def _gen_file_header(self):
        '''
        Generates the initial part of a source file, which may contain things such as
            includes, imports, comments to inform that the code was generated.
        '''
        raise NotImplementedError()

    def _gen_func_header(self, interface_name, method_name, input_params, output_params, return_type):
        '''
        Generates things such as return type, function name, list of arguments, opening brace
        '''
        raise NotImplementedError()

    def _gen_init_buf_params(self, i, expression):
        '''
        Generate call to seL4_SetMR(i, expression);
        '''
        raise NotImplementedError()

    def _gen_init_reg_params(self, i, expression):
        '''
        Generate assignment of the value of expression to ith reg
        '''
        raise NotImplementedError()

    def _gen_result(self, returning_struct: bool, return_type):
        '''
        Generate assignment to 'result' variable
        '''
        raise NotImplementedError()

    def _gen_result_struct(self, interface_name, method_name, output_params):
        """
        Generate a structure definition to be returned by the system call stubs to
        the user.

        We have a few constraints:

            * We always need an 'error' output parameter, even though it won't
              appear in the list 'output_params' given to us.

            * Output parameters may be marked as 'pass_by_reference', indicating
              that we only ever see pointers to the item.

        If no structure is needed (i.e., we just return an error code), we return
        a falsy string ('').
        """
        raise NotImplementedError()

    def _gen_return_end_function(self, returning_struct):
        '''
        Generates return statement and closes the body of a function
        '''
        raise NotImplementedError()

    def _gen_setup_input_capability(self, i, expression):
        '''
        Generate call to 'seL4_SetCap(i, expression);'
        '''
        raise NotImplementedError()

    def _gen_unmarshal_regs_into_ipc(self, num_mrs, returning_struct):
        raise NotImplementedError()

    def _gen_unmarshal_result(self, num_mrs, output_params):
        raise NotImplementedError()

    def init_data_types(self, BitFieldType, CapType, Type):
        wordsize = self.arch.wordsize
        return [
            # Simple Types
            Type("int", 32, wordsize),
            Type("long", wordsize, wordsize),

            Type("seL4_Uint8", 8, wordsize),
            Type("seL4_Uint16", 16, wordsize),
            Type("seL4_Uint32", 32, wordsize),
            Type("seL4_Uint64", 64, wordsize),
            Type("seL4_Time", 64, wordsize),
            Type("seL4_Word", wordsize, wordsize),
            Type("seL4_Bool", 1, wordsize, native_size_bits=8),

            # seL4 Structures
            BitFieldType("seL4_CapRights_t", wordsize, wordsize),

            # Object types
            CapType("seL4_CPtr", wordsize),
            CapType("seL4_CNode", wordsize),
            CapType("seL4_IRQHandler", wordsize),
            CapType("seL4_IRQControl", wordsize),
            CapType("seL4_TCB", wordsize),
            CapType("seL4_Untyped", wordsize),
            CapType("seL4_DomainSet", wordsize),
            CapType("seL4_SchedContext", wordsize),
            CapType("seL4_SchedControl", wordsize),
        ]

    def init_arch_types(self, CapType, StructType, Type):
        wordsize = self.arch.wordsize
        arm_smmu = [
            CapType("seL4_ARM_SIDControl", wordsize),
            CapType("seL4_ARM_SID", wordsize),
            CapType("seL4_ARM_CBControl", wordsize),
            CapType("seL4_ARM_CB", wordsize),
        ]
        arch_types = {
            "aarch32": [
                Type("seL4_ARM_VMAttributes", wordsize, wordsize),
                CapType("seL4_ARM_Page", wordsize),
                CapType("seL4_ARM_PageTable", wordsize),
                CapType("seL4_ARM_PageDirectory", wordsize),
                CapType("seL4_ARM_ASIDControl", wordsize),
                CapType("seL4_ARM_ASIDPool", wordsize),
                CapType("seL4_ARM_VCPU", wordsize),
                CapType("seL4_ARM_IOSpace", wordsize),
                CapType("seL4_ARM_IOPageTable", wordsize),
                StructType("seL4_UserContext", wordsize * 19, wordsize),
            ] + arm_smmu,

            "aarch64": [
                Type("seL4_ARM_VMAttributes", wordsize, wordsize),
                CapType("seL4_ARM_Page", wordsize),
                CapType("seL4_ARM_PageTable", wordsize),
                CapType("seL4_ARM_PageDirectory", wordsize),
                CapType("seL4_ARM_PageUpperDirectory", wordsize),
                CapType("seL4_ARM_PageGlobalDirectory", wordsize),
                CapType("seL4_ARM_VSpace", wordsize),
                CapType("seL4_ARM_ASIDControl", wordsize),
                CapType("seL4_ARM_ASIDPool", wordsize),
                CapType("seL4_ARM_VCPU", wordsize),
                CapType("seL4_ARM_IOSpace", wordsize),
                CapType("seL4_ARM_IOPageTable", wordsize),
                StructType("seL4_UserContext", wordsize * 36, wordsize),
            ] + arm_smmu,

            "ia32": [
                Type("seL4_X86_VMAttributes", wordsize, wordsize),
                CapType("seL4_X86_IOPort", wordsize),
                CapType("seL4_X86_IOPortControl", wordsize),
                CapType("seL4_X86_ASIDControl", wordsize),
                CapType("seL4_X86_ASIDPool", wordsize),
                CapType("seL4_X86_IOSpace", wordsize),
                CapType("seL4_X86_Page", wordsize),
                CapType("seL4_X86_PageDirectory", wordsize),
                CapType("seL4_X86_PageTable", wordsize),
                CapType("seL4_X86_IOPageTable", wordsize),
                CapType("seL4_X86_VCPU", wordsize),
                CapType("seL4_X86_EPTPML4", wordsize),
                CapType("seL4_X86_EPTPDPT", wordsize),
                CapType("seL4_X86_EPTPD", wordsize),
                CapType("seL4_X86_EPTPT", wordsize),
                StructType("seL4_VCPUContext", wordsize * 7, wordsize),
                StructType("seL4_UserContext", wordsize * 12, wordsize),
            ],

            "x86_64": [
                Type("seL4_X86_VMAttributes", wordsize, wordsize),
                CapType("seL4_X86_IOPort", wordsize),
                CapType("seL4_X86_IOPortControl", wordsize),
                CapType("seL4_X86_ASIDControl", wordsize),
                CapType("seL4_X86_ASIDPool", wordsize),
                CapType("seL4_X86_IOSpace", wordsize),
                CapType("seL4_X86_Page", wordsize),
                CapType("seL4_X64_PML4", wordsize),
                CapType("seL4_X86_PDPT", wordsize),
                CapType("seL4_X86_PageDirectory", wordsize),
                CapType("seL4_X86_PageTable", wordsize),
                CapType("seL4_X86_IOPageTable", wordsize),
                CapType("seL4_X86_VCPU", wordsize),
                CapType("seL4_X86_EPTPML4", wordsize),
                CapType("seL4_X86_EPTPDPT", wordsize),
                CapType("seL4_X86_EPTPD", wordsize),
                CapType("seL4_X86_EPTPT", wordsize),
                StructType("seL4_VCPUContext", wordsize * 7, wordsize),
                StructType("seL4_UserContext", wordsize * 20, wordsize),
            ],
            "riscv32": [
                Type("seL4_RISCV_VMAttributes", wordsize, wordsize),
                CapType("seL4_RISCV_Page", wordsize),
                CapType("seL4_RISCV_PageTable", wordsize),
                CapType("seL4_RISCV_ASIDControl", wordsize),
                CapType("seL4_RISCV_ASIDPool", wordsize),
                StructType("seL4_UserContext", wordsize * 32, wordsize),
            ],
            "riscv64": [
                Type("seL4_RISCV_VMAttributes", wordsize, wordsize),
                CapType("seL4_RISCV_Page", wordsize),
                CapType("seL4_RISCV_PageTable", wordsize),
                CapType("seL4_RISCV_ASIDControl", wordsize),
                CapType("seL4_RISCV_ASIDPool", wordsize),
                StructType("seL4_UserContext", wordsize * 32, wordsize),
            ]
        }
        # legacy alias
        arch_types["arm_hyp"] = arch_types["aarch32"]
        return arch_types[self.arch.name]

    def __init__(self, arch, xml_files):
        self.contents = []
        self.arch = arch
        self.files = xml_files
        self.arch_types = self.init_arch_types()
        self.data_types = self.init_data_types()

    def generate_marshal_expressions(self, params, num_mrs, structs):
        """
        Generate marshalling expressions for the given set of inputs.

        We return a list of expressions; one expression per word required
        to marshal all the inputs.
        """
        raise NotImplementedError()

    def generate_unmarshal_expressions(self, params):
        """
        Generate unmarshalling expressions for the given set of outputs.

        We return a list of list of expressions; one list per variable, containing
        expressions for the words in it that must be unmarshalled. The expressions
        will have tokens of the form:
            "%(w0)s"
        in them, indicating a read from a word in the message.
        """
        raise NotImplementedError()

    def get_func_return_type(self, returning_struct: bool, interface_name, method_name):
        """
        Returns the name of return type of a function
        """
        raise NotImplementedError()

    def generate(self, output_file):
        """
        Generate a header file containing system call stubs for seL4.
        """
        self.contents = []

        data_types = self.data_types
        arch_types = self.arch_types

        # Parse XML
        methods = []
        structs = []
        for infile in self.files:
            method, struct, _ = parse_xml_file(infile, data_types + arch_types)
            methods += method
            structs += struct

        # Print header.
        self.contents.append("")
        self._gen_comment(
            ["Automatically generated system call stubs."]
        )
        self.contents.append("")

        self._gen_file_header()

        #
        # Generate structures needed to return results back to the user.
        #
        # We can not use pass-by-reference (except for really large objects), as
        # the verification framework does not support them.
        #
        self._gen_comment(
            ["Return types for generated methods."]
        )
        for (interface_name, method_name, _, _, output_params, _, _) in methods:
            self._gen_result_struct(interface_name, method_name, output_params)
        #
        # Generate the actual stub code.
        #
        self._gen_comment(["Generated stubs."])
        for (interface_name, method_name, method_id, inputs, outputs, condition, comment_lines) in methods:
            # TODO condition is specific to C, restructured for Rust
            # it should be language agnostic upfront

            condition_end = self._gen_conditional_compilation(condition)
            self.generate_stub(interface_name, method_name, method_id, inputs,
                               outputs, structs, comment_lines)
            if condition_end:
                self.contents.append(condition_end)

        self._gen_file_footer()

        # Write the output
        output = open(output_file, "w")
        output.write("\n".join(self.contents))
        output.close()

    def generate_stub(self, interface_name, method_name, method_id, input_params, output_params, structs, comment_lines):
        num_mrs = self.arch.message_registers

        # Split out cap parameters and standard parameters
        standard_params = []
        cap_params = []
        for x in input_params:
            # TODO check this sneaky case, it retrieves a static class from the specific Generator implementation
            # type(self).CapType should be CGenerator.CapType or RustGenerator.CapType
            # depending on which generator was created
            if isinstance(x.type, type(self).CapType):
                cap_params.append(x)
            else:
                standard_params.append(x)

        # Determine if we are returning a structure, or just the error code.
        returning_struct: bool = is_result_struct_required(output_params)

        return_type = self.get_func_return_type(returning_struct, interface_name, method_name)

        #
        # Print doxygen comment.
        #
        self._gen_comment(comment_lines, doc=True)

        #
        # Print function header.
        #
        self._gen_func_header(interface_name, method_name, input_params, output_params, return_type)

        #
        # Get a list of expressions for our caps and inputs.
        #
        input_expressions = self.generate_marshal_expressions(standard_params,
                                                              num_mrs, structs)

        cap_expressions = [x.name for x in cap_params]
        service_cap = cap_expressions[0]
        cap_expressions = cap_expressions[1:]

        #
        # Compute how many words the inputs and output will require.
        #
        # TODO do we need these? unused
        input_param_words = len(input_expressions)
        output_param_words = sum([p.type.size_bits for p in output_params]) // self.arch.wordsize

        #
        # Setup variables we will need.
        #
        self._gen_declare_variables(returning_struct, return_type, method_id,
                                    cap_expressions, input_expressions, num_mrs)

        self.contents.append("")

        #
        # Copy capabilities.
        #
        #   /* Setup input capabilities. */
        #   seL4_SetCap(i, cap);
        #
        if len(cap_expressions) > 0:
            self._gen_comment(["Setup input capabilities."], indent=1, inline=True)
            for i in range(len(cap_expressions)):
                self._gen_setup_input_capability(i, cap_expressions[i])
            self.contents.append("")

        #
        # Copy in the inputs.
        #
        #   /* Marshal input parameters. */
        #   seL4_SetMR(i, v);
        #   ...
        #
        if max(num_mrs, len(input_expressions)) > 0:
            self._gen_comment(["Marshal and initialise parameters."], indent=1, inline=True)
            # Initialize in-register parameters
            for i in range(num_mrs):
                if i < len(input_expressions):
                    self._gen_init_reg_params(i, input_expressions[i])
                else:
                    self._gen_init_reg_params(i, 0)

            # Initialize buffered parameters
            for i in range(num_mrs, len(input_expressions)):
                self._gen_init_buf_params(i, input_expressions[i])
            self.contents.append("")

        #
        # Generate the call.
        #
        if self.arch.use_only_ipc_buffer:
            self._gen_comment(["Perform the call."], indent=1, inline=True)
        else:
            self._gen_comment(["Perform the call, passing in-register arguments directly."],
                              indent=1, inline=True)
        self._gen_call(service_cap, num_mrs)

        #
        # Prepare the result.
        #
        self._gen_result(returning_struct, return_type)
        self.contents.append("")

        if not self.arch.use_only_ipc_buffer:
            self._gen_comment(
                ["Unmarshal registers into IPC buffer on error."],
                indent=1, inline=True)
            self._gen_unmarshal_regs_into_ipc(num_mrs, returning_struct)
            self.contents.append("")

        #
        # Generate unmarshalling code.
        #
        if len(output_params) > 0:
            self._gen_comment(["Unmarshal result."], indent=1, inline=True)
            self._gen_unmarshal_result(num_mrs, output_params, structs)
        #
        # }
        #
        self._gen_return_end_function(returning_struct)
        self.contents.append("")
