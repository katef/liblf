/*
 * Copyright 2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <lf/lf.h>

static int
print_literal(char c)
{
	printf("literal: \\x%02X\n", (unsigned char) c);
	return 1;
}

static int
print_ip(enum lf_ip ip)
{
	const char *s;

	switch (ip) {
	case LF_IP_CLIENT: s = "client"; break;
	case LF_IP_PEER:   s = "peer";   break;
	case LF_IP_LOCAL:  s = "local";  break;

	default:
		assert(!"unreached");
		abort();
	}

	printf("ip (%s)\n", s);
	return 1;
}

static int
print_resp_size(void)
{
	printf("resp_size\n");
	return 1;
}

static int
print_resp_size_clf(void)
{
	printf("resp_size_clf\n");
	return 1;
}

static int
print_req_cookie(const char *name)
{
	printf("req_cookie: %s\n", name);
	return 1;
}

static int
print_env_var(const char *name)
{
	printf("env_var: %s\n", name);
	return 1;
}

static int
print_filename(void)
{
	printf("filename\n");
	return 1;
}

static int
print_remote_hostname(int hostname_lookups)
{
	printf("remote_hostname (hostname_lookups=%s)\n",
		hostname_lookups ? "true" : "false");
	return 1;
}

static int
print_req_protocol(void)
{
	printf("req_header\n");
	return 1;
}

static int
print_req_header(const char *name)
{
	printf("req_header: %s\n", name);
	return 1;
}

static int
print_keepalive_reqs(void)
{
	printf("keepalive_reqs\n");
	return 1;
}

static int
print_remote_logname(void)
{
	printf("remote_logname\n");
	return 1;
}

static int
print_req_logid(void)
{
	printf("req_logid\n");
	return 1;
}

static int
print_req_method(void)
{
	printf("req_method\n");
	return 1;
}

static int
print_note(const char *name)
{
	printf("note: %s\n", name);
	return 1;
}

static int
print_reply_header(const char *name)
{
	printf("reply_header: %s\n", name);
	return 1;
}

static int
print_server_port(enum lf_port port)
{
	const char *s;

	switch (port) {
	case LF_PORT_CANONICAL: s = "canonical"; break;
	case LF_PORT_LOCAL:     s = "local";     break;
	case LF_PORT_REMOTE:    s = "remote";    break;

	default:
		assert(!"unreached");
		abort();
	}

	printf("server_port (%s)\n", s);
	return 1;
}

static int
print_id(enum lf_id id)
{
	const char *s;

	switch (id) {
	case LF_ID_PID:    s = "pid";    break;
	case LF_ID_TID:    s = "tid";    break;
	case LF_ID_HEXTID: s = "hextid"; break;

	default:
		assert(!"unreached");
		abort();
	}

	printf("id (%s)\n", s);
	return 1;
}

static int
print_query_string(void)
{
	printf("query_string\n");
	return 1;
}

static int
print_req_first_line(void)
{
	printf("req_first_line\n");
	return 1;
}

static int
print_resp_handler(void)
{
	printf("resp_handler\n");
	return 1;
}

static int
print_status(void)
{
	printf("status\n");
	return 1;
}

static int
print_strftime(enum lf_when when, const char *fmt)
{
	printf("strftime: (when=%d, fmt=%s)\n", when, fmt);
	return 1;
}

static int
print_time_frac(enum lf_when when, enum lf_rtime unit)
{
	const char *s;

	switch (unit) {
	case LF_RTIME_MS_FRAC: s = "%ms"; break;
	case LF_RTIME_US_FRAC: s = "%us"; break;
	case LF_RTIME_MS:      s = "ms";  break;
	case LF_RTIME_US:      s = "us";  break;
	case LF_RTIME_S:       s = "s";   break;

	default:
		assert(!"unreached");
		abort();
	}

	printf("time_frac (when=%d, unit=%s)\n", when, s);
	return 1;
}

static int
print_time_taken(enum lf_rtime unit)
{
	const char *s;

	switch (unit) {
	case LF_RTIME_MS_FRAC:
	case LF_RTIME_US_FRAC:
		/* fractions are never present for .time_taken */
		assert(!"unreached");
		abort();

	case LF_RTIME_MS: s = "ms"; break;
	case LF_RTIME_US: s = "us"; break;
	case LF_RTIME_S:  s = "s";  break;

	default:
		assert(!"unreached");
		abort();
	}

	printf("time_taken (unit=%s)\n", s);
	return 1;
}

static int
print_remote_user(void)
{
	printf("remote_user\n");
	return 1;
}

static int
print_url_path(void)
{
	printf("url_path\n");
	return 1;
}

static int
print_server_name(int use_canonical_name)
{
	printf("server_name: use_canonical_name=%s\n",
		use_canonical_name ? "true" : "false");
	return 1;
}

static int
print_conn_status(void)
{
	printf("conn_status\n");
	return 1;
}

static int
print_bytes_recv(void)
{
	printf("bytes_recv\n");
	return 1;
}

static int
print_bytes_sent(void)
{
	printf("bytes_sent\n");
	return 1;
}

static int
print_bytes_xfer(void)
{
	printf("bytes_xfer\n");
	return 1;
}

static int
print_req_trailer(const char *name)
{
	printf("req_trailer: %s\n", name);
	return 1;
}

static int
print_resp_trailer(const char *name)
{
	printf("resp_trailer: %s\n", name);
	return 1;
}

int
main(int argc, char *argv[])
{
	struct lf_config conf;
	struct lf_err err;
	char buf[128];

	(void) argc;
	(void) argv; /* TODO */

/* TODO:
	conf.opaque    = NULL;
*/

	conf.keep_alive         = 0;
	conf.hostname_lookups   = 0;
	conf.identity_check     = 0;
	conf.use_canonical_name = 0;

	conf.buf   = buf;
	conf.bufsz = sizeof buf;

	conf.literal            = print_literal;

	conf.ip                 = print_ip;
	conf.resp_size          = print_resp_size;
	conf.resp_size_clf      = print_resp_size_clf;
	conf.req_cookie         = print_req_cookie;
	conf.env_var            = print_env_var;
	conf.filename           = print_filename;
	conf.remote_hostname    = print_remote_hostname;
	conf.req_protocol       = print_req_protocol;
	conf.req_header         = print_req_header;
	conf.keepalive_reqs     = print_keepalive_reqs;
	conf.remote_logname     = print_remote_logname;
	conf.req_logid          = print_req_logid;
	conf.req_method         = print_req_method;
	conf.note               = print_note;
	conf.reply_header       = print_reply_header;
	conf.server_port        = print_server_port;
	conf.id                 = print_id;
	conf.query_string       = print_query_string;
	conf.req_first_line     = print_req_first_line;
	conf.resp_handler       = print_resp_handler;
	conf.status             = print_status;
	conf.time               = print_strftime;
	conf.time_frac          = print_time_frac;
	conf.time_taken         = print_time_taken;
	conf.remote_user        = print_remote_user;
	conf.url_path           = print_url_path;
	conf.server_name        = print_server_name;
	conf.conn_status        = print_conn_status;
	conf.bytes_recv         = print_bytes_recv;
	conf.bytes_sent         = print_bytes_sent;
	conf.bytes_xfer         = print_bytes_xfer;
	conf.req_trailer        = print_req_trailer;
	conf.resp_trailer       = print_resp_trailer;

	if (!lf_parse(&conf, argv[1], &err)) {
		assert(err.e >= err.p);
		printf("error: %d\n", err.errnum); /* XXX */
		return 1;
	}

	return 0;
}

