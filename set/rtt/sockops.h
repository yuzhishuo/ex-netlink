#pragma once
#ifndef LULUYUZHI_RTT_SOCKOPS_H
#define LULUYUZHI_RTT_SOCKOPS_H
#include <sys/uio.h>

namespace luluyuzhi
{
    // ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
    // ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
    // struct iovec {
    //     void  *iov_base;    /* Starting address */
    //     size_t iov_len;     /* Number of bytes to transfer */
    // };

    
} // namespace luluyuzhi


#endif // LULUYUZHI_RTT_SOCKOPS_H