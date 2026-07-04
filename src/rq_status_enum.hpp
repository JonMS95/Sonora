#ifndef RQ_STATUS_ENUM_HPP
#define RQ_STATUS_ENUM_HPP

enum class request_status_t
{
    OP_QUEUED  = 0,
    OP_ONGOING    ,
    OP_OK         ,
    OP_ERROR      ,
    OP_UNKNOWN    ,
};

#endif
