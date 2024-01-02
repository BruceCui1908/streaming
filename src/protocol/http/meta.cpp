#include "meta.h"

namespace http {

const char *code_to_msg(code status)
{
    switch (status)
    {
    case Method_Not_Allowed:
        return "Method Not Allowed";

    default:
        return "";
    }
}
} // namespace http