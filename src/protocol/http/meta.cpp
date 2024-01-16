#include "meta.h"

namespace http {

const char *code_to_msg(Status status)
{
    switch (status)
    {
    case Status::OK:
        return "OK";
    case Status::Bad_Request:
        return "Bad Request";
    case Status::Method_Not_Allowed:
        return "Method Not Allowed";
    case Status::Unsupported_Media_Type:
        return "Unsupported Media Type";
    case Status::Not_Found:
        return "Not Found";
    case Status::Internal_Server_Error:
        return "Internal Server Error";
    default:
        return "";
    }
}
} // namespace http