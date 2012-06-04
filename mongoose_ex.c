// Copyright (c) 2011 Ger Hobbelt
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


// 'Extends' mongoose by exporting additional functions

#include "mongoose_ex.h"

#include "mongoose.c"


struct socket *mg_get_socket(struct mg_connection *conn)
{
    return conn ? &conn->client : NULL;
}

int64_t mg_get_num_bytes_sent(struct mg_connection *conn)
{
    return conn ? conn->num_bytes_sent > 0 ? conn->num_bytes_sent : 0 : 0;
}

int64_t mg_get_num_bytes_received(struct mg_connection *conn)
{
    return conn ? conn->consumed_content : 0;
}

int mg_set_non_blocking_mode(struct mg_connection *conn, int on)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
        return !!set_non_blocking_mode(conn->client.sock, on);
    return -1;
}

int mg_shutdown(struct mg_connection *conn, int how)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
        return shutdown(conn->client.sock, how);
    return -1;
}

int mg_setsockopt(struct mg_connection *conn, int level, int optname, const void *optval, size_t optlen)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
        return setsockopt(conn->client.sock, level, optname, optval, optlen);
    return -1;
}

int mg_getsockopt(struct mg_connection *conn, int level, int optname, void *optval, size_t *optlen_ref)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
    {
        socklen_t optlen = 0;
        int rv = getsockopt(conn->client.sock, level, optname, optval, &optlen);

        *optlen_ref = optlen;
        return rv;
    }
    return -1;
}


// http://www.unixguide.net/network/socketfaq/2.11.shtml
// http://www.techrepublic.com/article/tcpip-options-for-high-performance-data-transmission/1050878
int mg_set_nodelay_mode(struct mg_connection *conn, int on)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
    {
#if !defined(SOL_TCP) && (defined(_WIN32) && !defined(__SYMBIAN32__))
        DWORD v_on = !!on;
        return setsockopt(conn->client.sock, IPPROTO_TCP, TCP_NODELAY, (void *)&v_on, sizeof(v_on));
#elif !defined(SOL_TCP)
        int v_on = !!on;
        return setsockopt(conn->client.sock, IPPROTO_TCP, TCP_NODELAY, (void *)&v_on, sizeof(v_on));
#else
        int v_on = !!on;
        return setsockopt(conn->client.sock, SOL_TCP, TCP_NODELAY, (void *)&v_on, sizeof (v_on));
#endif
    }
    return -1;
}

int mg_set_socket_keepalive(struct mg_connection *conn, int on)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
    {
#if defined(_WIN32) && !defined(__SYMBIAN32__)
        BOOL v_on = !!on;
#else
        int v_on = !!on;
#endif
        return setsockopt(conn->client.sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&v_on, sizeof(v_on));
    }
    return -1;
}

int mg_set_socket_timeout(struct mg_connection *conn, int seconds)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
        return set_timeout(&conn->client, seconds);
    return -1;
}

/*
ntop()/ntoa() replacement with IPv6 + IPv4 support.

remote = 1 will print the remote IP address, while
remote = 0 will print the local IP address

'dst' is also returned as function result; on error, 'dst' will
contain an empty string.
*/
char *mg_sockaddr_to_string(char *dst, size_t dstlen, const struct mg_connection *conn, int remote)
{
    if (!dst) return NULL;
    dst[0] = 0;

    if (conn && conn->client.sock != INVALID_SOCKET)
    {
        char src_addr[SOCKADDR_NTOA_BUFSIZE];

        sockaddr_to_string(src_addr, sizeof(src_addr), (remote ? &conn->client.rsa : &conn->client.lsa));
        // only copy IP address in its entirety or not at all:
        if (dstlen > strlen(src_addr))
        {
            strcpy(dst, src_addr);
        }
    }
    return dst;
}

/*
ntoh() replacement for IPv6 + IPv4 support.

remote = 1 will produce the remote port, while
remote = 0 will produce the local port for the given connection (socket)
*/
unsigned short int mg_get_socket_port(const struct mg_connection *conn, int remote)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
    {
        if (remote)
            return get_socket_port(&conn->client.rsa);
        else
            return get_socket_port(&conn->client.lsa);
    }
    return 0;
}

/*
IPv4 + IPv6 support: produce the individual numbers of the IP address in a usable/portable (host) structure

remote = 1 will print the remote IP address, while
remote = 0 will print the local IP address

Return 0 on success, non-zero on error.
*/
int mg_get_socket_ip_address(struct mg_ip_address *dst, const struct mg_connection *conn, int remote)
{
    if (dst && conn && conn->client.sock != INVALID_SOCKET)
    {
        if (remote)
            get_socket_ip_address(dst, &conn->client.rsa);
        else
            get_socket_ip_address(dst, &conn->client.lsa);
        return 0;
    }
    return -1;
}

void mg_FD_SET(struct mg_connection *conn, fd_set *set, int *max_fd)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
        add_to_set(conn->client.sock, set, max_fd);
}

int mg_FD_ISSET(struct mg_connection *conn, fd_set *set)
{
    if (conn && conn->client.sock != INVALID_SOCKET)
        return FD_ISSET(conn->client.sock, set);
    return 0;
}

struct mg_connection *mg_connect_to_host(struct mg_context *ctx, const char *host, int port, int use_ssl)
{
    struct mg_connection *conn = fc(ctx);

    return mg_connect(conn, host, port, use_ssl);
}

int mg_pull(struct mg_connection *conn, void *buf, size_t max_bufsize)
{
    int buffered_len, nread;

    assert((conn->content_len == -1 && conn->consumed_content == 0) ||
        (conn->content_len == 0 && conn->consumed_content > 0) ||
        conn->consumed_content <= conn->content_len);
    DEBUG_TRACE(("%p %" INT64_FMT " %" INT64_FMT " %" INT64_FMT, buf, (int64_t)max_bufsize,
        conn->content_len, conn->consumed_content));
    nread = 0;
    if (conn->consumed_content < conn->content_len)
    {
        // How many bytes of data we have buffered in the request buffer?
        buffered_len = conn->data_len - conn->request_len;
        assert(buffered_len >= 0);

        // Return buffered data back if we haven't done that yet.
        if (conn->consumed_content < (int64_t) buffered_len) {
            buffered_len -= (int) conn->consumed_content;
            if (max_bufsize < (size_t) buffered_len) {
                buffered_len = max_bufsize;
            }
            nread = mg_read(conn, buf, buffered_len);
            buf = (char *) buf + buffered_len;
            max_bufsize -= buffered_len;
        }
    }

    // We have returned all buffered data. Read new data from the remote socket.
    if (max_bufsize > 0) {
        int n = pull(NULL, conn->client.sock, conn->ssl, (char *) buf, (int) max_bufsize);
        if (n > 0) {
            conn->consumed_content += n;
            nread += n;
        }
    }
    return nread;
}

void mg_close_connection(struct mg_connection *conn)
{
    close_connection(conn);
    free(conn);
}






// now here is a prime candidate for C++ polymorphism...

void mg_cry4ctx(struct mg_context *ctx, const char *fmt, ...)
{
    time_t timestamp = time(NULL);
    va_list ap;

    va_start(ap, fmt);
    mg_vwrite2log(fc(ctx), NULL, timestamp, NULL, fmt, ap);
    va_end(ap);
}

void mg_log(struct mg_connection *conn, const char *severity, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  mg_vlog(conn, severity, fmt, ap);
  va_end(ap);
}

// Print error message to the opened error log stream.
void mg_vlog(struct mg_connection *conn, const char *severity, const char *fmt, va_list args)
{
    time_t timestamp = time(NULL);

    mg_vwrite2log(conn, NULL, timestamp, severity, fmt, args);
}



int mg_get_lasterror(void)
{
    return ERRNO;
}



int mg_start_thread(struct mg_context *ctx, mg_thread_func_t func, void *param)
{
    int rv = start_thread(ctx, func, param);
    if (rv == 0)
    {
        // count this thread too so the master_thread will wait for this one to end as well when we stop.
        (void) pthread_mutex_lock(&ctx->mutex);
        ctx->num_threads++;
        (void) pthread_mutex_unlock(&ctx->mutex);
    }
    return rv;
}

void mg_signal_mgr_this_thread_is_done(struct mg_context *ctx)
{
    (void) pthread_mutex_lock(&ctx->mutex);
    ctx->num_threads--;
    (void) pthread_cond_signal(&ctx->cond);
    assert(ctx->num_threads >= 0);
    (void) pthread_mutex_unlock(&ctx->mutex);
}




int mg_match_prefix(const char *pattern, int pattern_len, const char *str)
{
    if (!str || !pattern) return -1;

    return match_prefix(pattern, pattern_len, str);
}

time_t mg_parse_date_string(const char *datetime)
{
    if (!datetime)
        return (time_t)0;

    return parse_date_string(datetime);
}


static void set_header_ptr(const char **dst, int dstsize, int idx, const char *value)
{
    if (dst && idx >= 0 && idx < dstsize)
    {
        dst[idx] = value;
    }
}

int mg_get_headers(const char **dst, int dst_buffersize, const struct mg_connection *conn, const char *name)
{
    int i;
    int cnt = 0;
    const struct mg_request_info *ri = &conn->request_info;

    set_header_ptr(dst, dst_buffersize, 0, NULL);
    for (i = 0; i < ri->num_headers; i++)
    {
        if (!mg_strcasecmp(name, ri->http_headers[i].name))
        {
            set_header_ptr(dst, dst_buffersize, cnt++, ri->http_headers[i].value);
        }
    }
    set_header_ptr(dst, dst_buffersize, cnt, NULL);
    set_header_ptr(dst, dst_buffersize, dst_buffersize - 1, NULL);

    return cnt;
}

void mg_send_http_error(struct mg_connection *conn, int status, const char *reason, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsend_http_error(conn, status, reason, fmt, ap);
    va_end(ap);
}

void mg_vsend_http_error(struct mg_connection *conn, int status, const char *reason, const char *fmt, va_list ap)
{
    vsend_http_error(conn, status, reason, fmt, ap);
}

void mg_gmt_time_string(char *buf, size_t bufsize, const time_t *tm)
{
    gmt_time_string(buf, bufsize, tm);
}

const char *mg_suggest_connection_header(struct mg_connection *conn)
{
    return suggest_connection_header(conn);
}

void mg_connection_must_close(struct mg_connection *conn)
{
    conn->must_close = 1;
}




#include "selectable-socketpair/socketpair.c"

int mg_socketpair(struct mg_connection *conns[2], struct mg_context *ctx)
{
    int rv = -1;
    if (conns)
    {
        int i;
#ifdef WIN32
        SOCKET socks[2];
#else
        int socks[2];
#endif

        conns[0] = (struct mg_connection *)calloc(1, sizeof(*conns[0]));
        conns[1] = (struct mg_connection *)calloc(1, sizeof(*conns[1]));
        if (!conns[0] || !conns[1])
        {
            mg_cry(fc(ctx), "%s: calloc: %s", __func__, mg_strerror(ERRNO));
        }
        else
        {
            rv = dumb_socketpair(socks, 0);
            if (rv)
            {
                mg_cry(fc(ctx), "%s: socketpair: %s", __func__, mg_strerror(ERRNO));
            }
            else
            {
                for (i = 0; i <= 1; i++)
                {
                    struct mg_connection *newconn = conns[i];

                    newconn->birth_time = time(NULL);
                    newconn->ctx = ctx;
                    newconn->client.sock = socks[i];
                    newconn->client.rsa.u.sin.sin_family = AF_INET;
                    newconn->client.rsa.u.sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                    newconn->client.rsa.u.sin.sin_port = 0;
                    newconn->client.rsa.len = sizeof(newconn->client.rsa.u.sin);
                    newconn->client.lsa.len = sizeof(newconn->client.lsa.u);
                    if (0 != getsockname(socks[i], &newconn->client.lsa.u.sa, &newconn->client.lsa.len))
                    {
                        mg_cry(newconn, "%s: getsockname: %s", __func__, mg_strerror(ERRNO));
                        newconn->client.lsa.len = 0;
                        rv = -1;
                    }
                }

                if (rv)
                {
                    closesocket(socks[0]);
                    closesocket(socks[1]);
                }
            }
        }

        if (rv)
        {
            free(conns[0]);
            free(conns[1]);
            conns[0] = conns[1] = NULL;
        }
    }
    return rv;
}
