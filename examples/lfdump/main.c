/*
 * Copyright 2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <lf/lf.h>

static void
print_pred(const struct lf_pred *pred)
{
	size_t i;

	assert(pred != NULL);

	if (pred->count == 0) {
		return;
	}

	if (pred->neg) {
		printf("!");
	}

	for (i = 0; i < pred->count; i++) {
		printf("%u", pred->status[i]);

		if (i + 1 < pred->count) {
			printf(",");
		}
	}

	printf(": ");
}

static void
print_redirect(enum lf_redirect redirect)
{
	switch (redirect) {
	case LF_REDIRECT_ORIG:  printf("<"); break;
	case LF_REDIRECT_FINAL: printf(">"); break;

	default:
		assert(!"unreached");
		abort();
	}
}

static int
print_literal(void *opaque, char c)
{
	assert(opaque == NULL);

	printf("literal: \\x%02X\n", (unsigned char) c);
	return 1;
}

static int
print_ip(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, enum lf_ip ip)
{
	const char *s;

	assert(opaque == NULL);

	switch (ip) {
	case LF_IP_CLIENT: s = "client"; break;
	case LF_IP_PEER:   s = "peer";   break;
	case LF_IP_LOCAL:  s = "local";  break;

	default:
		assert(!"unreached");
		abort();
	}

	print_pred(pred);
	print_redirect(redirect);
	printf("ip (%s)\n", s);
	return 1;
}

static int
print_resp_size(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("resp_size\n");
	return 1;
}

static int
print_resp_size_clf(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("resp_size_clf\n");
	return 1;
}

static int
print_req_cookie(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, const char *name)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("req_cookie: %s\n", name);
	return 1;
}

static int
print_env_var(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, const char *name)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("env_var: %s\n", name);
	return 1;
}

static int
print_filename(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("filename\n");
	return 1;
}

static int
print_remote_hostname(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, int hostname_lookups)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("remote_hostname (hostname_lookups=%s)\n",
		hostname_lookups ? "true" : "false");
	return 1;
}

static int
print_req_protocol(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("req_header\n");
	return 1;
}

static int
print_req_header(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, const char *name)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("req_header: %s\n", name);
	return 1;
}

static int
print_keepalive_reqs(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("keepalive_reqs\n");
	return 1;
}

static int
print_remote_logname(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("remote_logname\n");
	return 1;
}

static int
print_req_logid(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("req_logid\n");
	return 1;
}

static int
print_req_method(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("req_method\n");
	return 1;
}

static int
print_note(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, const char *name)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("note: %s\n", name);
	return 1;
}

static int
print_reply_header(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, const char *name)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("reply_header: %s\n", name);
	return 1;
}

static int
print_server_port(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, enum lf_port port)
{
	const char *s;

	assert(opaque == NULL);

	switch (port) {
	case LF_PORT_CANONICAL: s = "canonical"; break;
	case LF_PORT_LOCAL:     s = "local";     break;
	case LF_PORT_REMOTE:    s = "remote";    break;

	default:
		assert(!"unreached");
		abort();
	}

	print_pred(pred);
	print_redirect(redirect);
	printf("server_port (%s)\n", s);
	return 1;
}

static int
print_id(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, enum lf_id id)
{
	const char *s;

	assert(opaque == NULL);

	switch (id) {
	case LF_ID_PID:    s = "pid";    break;
	case LF_ID_TID:    s = "tid";    break;
	case LF_ID_HEXTID: s = "hextid"; break;

	default:
		assert(!"unreached");
		abort();
	}

	print_pred(pred);
	print_redirect(redirect);
	printf("id (%s)\n", s);
	return 1;
}

static int
print_query_string(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("query_string\n");
	return 1;
}

static int
print_req_first_line(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("req_first_line\n");
	return 1;
}

static int
print_resp_handler(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("resp_handler\n");
	return 1;
}

static int
print_status(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("status\n");
	return 1;
}

static int
print_strftime(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, enum lf_when when, const char *fmt)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("strftime: (when=%d, fmt=%s)\n", when, fmt);
	return 1;
}

static int
print_time_frac(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, enum lf_when when, enum lf_rtime unit)
{
	const char *s;

	assert(opaque == NULL);

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

	print_pred(pred);
	print_redirect(redirect);
	printf("time_frac (when=%d, unit=%s)\n", when, s);
	return 1;
}

static int
print_time_taken(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, enum lf_rtime unit)
{
	const char *s;

	assert(opaque == NULL);

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

	print_pred(pred);
	print_redirect(redirect);
	printf("time_taken (unit=%s)\n", s);
	return 1;
}

static int
print_remote_user(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("remote_user\n");
	return 1;
}

static int
print_url_path(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("url_path\n");
	return 1;
}

static int
print_server_name(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, int use_canonical_name)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("server_name: use_canonical_name=%s\n",
		use_canonical_name ? "true" : "false");
	return 1;
}

static int
print_conn_status(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("conn_status\n");
	return 1;
}

static int
print_bytes_recv(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("bytes_recv\n");
	return 1;
}

static int
print_bytes_sent(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("bytes_sent\n");
	return 1;
}

static int
print_bytes_xfer(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("bytes_xfer\n");
	return 1;
}

static int
print_req_trailer(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, const char *name)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("req_trailer: %s\n", name);
	return 1;
}

static int
print_resp_trailer(void *opaque, const struct lf_pred *pred,
	enum lf_redirect redirect, const char *name)
{
	assert(opaque == NULL);

	print_pred(pred);
	print_redirect(redirect);
	printf("resp_trailer: %s\n", name);
	return 1;
}

int
main(int argc, char *argv[])
{
	struct lf_config conf;
	struct lf_err err;

	(void) argc;
	(void) argv; /* TODO */

	conf.keep_alive         = 0;
	conf.hostname_lookups   = 0;
	conf.identity_check     = 0;
	conf.use_canonical_name = 0;

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

	if (!lf_parse(&conf, NULL, argv[1], &err)) {
		printf("error: %s\n", lf_strerror(err.errnum)); /* XXX */
		return 1;
	}

	return 0;
}

