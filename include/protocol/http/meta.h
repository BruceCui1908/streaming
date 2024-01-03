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
    OK = 200,
    Bad_Request = 400,
    Not_Found = 404,
    Method_Not_Allowed = 405,
    Unsupported_Media_Type = 415,
    Internal_Server_Error = 500,
};

const char *code_to_msg(code);

static constexpr char k404Body[] = "<html>"
                                   "<head><title>404 Not Found</title></head>"
                                   "<body bgcolor=\"white\">"
                                   "<center><h1>您访问的资源不存在！</h1></center>"
                                   "<hr><center>Not Found!"
                                   "</center>"
                                   "</body>"
                                   "</html>";

} // namespace http