#pragma once

namespace http {

enum Method
{
    GET,
    POST,
    INVALID,
};

enum code
{
    Method_Not_Allowed = 405,
};

const char *code_to_msg(code);

} // namespace http