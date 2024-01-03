#include "meta.h"

namespace http {

const char *code_to_msg(code status)
{
    switch (status)
    {
    case OK:
        return "OK";
    case Bad_Request:
        return "Bad Request";
    case Method_Not_Allowed:
        return "Method Not Allowed";
    case Unsupported_Media_Type:
        return "Unsupported Media Type";
    case Not_Found:
        return "Not Found";
    case Internal_Server_Error:
        return "Internal Server Error";
    default:
        return "";
    }
}
} // namespace http