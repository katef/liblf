/*
 * Copyright 2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef LIBLF_H
#define LIBLF_H

enum lf_errno {
	LF_ERR_MISSING_CLOSING_BRACE,
	LF_ERR_MISSING_DIRECTIVE,
	LF_ERR_MISSING_ESCAPE,
	LF_ERR_MISSING_NAME,

	LF_ERR_UNRECOGNISED_DIRECTIVE,
	LF_ERR_UNRECOGNISED_ESCAPE,
	LF_ERR_UNRECOGNISED_IP_TYPE,
	LF_ERR_UNRECOGNISED_RTIME_UNIT,
	LF_ERR_UNRECOGNISED_PORT_TYPE,
	LF_ERR_UNRECOGNISED_ID_TYPE,

	LF_ERR_NAME_OVERFLOW,
	LF_ERR_STATUS_OVERFLOW,
	LF_ERR_TOO_MANY_STATUSES,
	LF_ERR_TOO_MANY_REDIRECT_FLAGS,
	LF_ERR_NAME_ALREADY_SET,
	LF_ERR_EMPTY_NAME,

	LF_ERR_HOOK
};

struct lf_err {
	enum lf_errno errnum;
	const char *p;
	const char *e;
};

enum lf_ip {
	LF_IP_CLIENT,
	LF_IP_PEER,
	LF_IP_LOCAL
};

enum lf_port {
	LF_PORT_CANONICAL,
	LF_PORT_LOCAL,
	LF_PORT_REMOTE
};

enum lf_rtime {
	LF_RTIME_MS_FRAC,
	LF_RTIME_US_FRAC,
	LF_RTIME_MS,
	LF_RTIME_US,
	LF_RTIME_S
};

enum lf_redirect {
	LF_REDIRECT_ORIG, /* < */
	LF_REDIRECT_FINAL /* > */
};

enum lf_id {
	LF_ID_PID,
	LF_ID_TID,
	LF_ID_HEXTID
};

enum lf_when {
	LF_WHEN_BEGIN,
	LF_WHEN_END
};

struct lf_pred {
	unsigned neg :1;
	size_t count;
	unsigned *status; /* array, sorted */
};

typedef int (lf_bool    )(const struct lf_pred *pred, int v);
typedef int (lf_char    )(char c);
typedef int (lf_ip      )(const struct lf_pred *pred, enum lf_ip ip);
typedef int (lf_simple  )(const struct lf_pred *pred);
typedef int (lf_rtime   )(const struct lf_pred *pred, enum lf_rtime unit);
typedef int (lf_name    )(const struct lf_pred *pred, const char *name);
typedef int (lf_port    )(const struct lf_pred *pred, enum lf_port);
typedef int (lf_id      )(const struct lf_pred *pred, enum lf_id);
typedef int (lf_strftime)(const struct lf_pred *pred, enum lf_when when, const char *fmt);
typedef int (lf_fractime)(const struct lf_pred *pred, enum lf_when when, enum lf_rtime unit);

/*
 * https://httpd.apache.org/docs/current/mod/mod_log_config.html#logformat
 */
struct lf_config {
	unsigned keep_alive         :1;
	unsigned hostname_lookups   :1;
	unsigned identity_check     :1; /* TODO: by callback */
	unsigned use_canonical_name :1;

	/*
	 * User-supplied storage for null-terminated strings
	 * for VARNAME, format, etc.
	 */
	char *buf;
	size_t bufsz;

	lf_char     *literal;

	lf_ip       *ip;                /* %a, %{c}a, %A */
	lf_simple   *resp_size;         /* %B */
	lf_simple   *resp_size_clf;     /* %b */
	lf_name     *req_cookie;        /* %{VARNAME}C */
	                                /* %D: see %T{us} */
	lf_name     *env_var;           /* %{VARNAME}e */
	lf_simple   *filename;          /* %f */
	lf_bool     *remote_hostname;   /* %h */
	lf_simple   *req_protocol;      /* %H */
	lf_name     *req_header;        /* %{VARNAME}i */
	lf_simple   *keepalive_reqs;    /* %k */
	lf_simple   *remote_logname;    /* %l */
	lf_simple   *req_logid;         /* %L */
	lf_simple   *req_method;        /* %m */
	lf_name     *note;              /* %{VARNAME}n */
	lf_name     *reply_header;      /* %{VARNAME}o */
	lf_port     *server_port;       /* %p, %{format}p */
	lf_id       *id;                /* %P, %{format}P */
	lf_simple   *query_string;      /* %q */
	lf_simple   *req_first_line;    /* %r */
	lf_simple   *resp_handler;      /* %R */
	lf_simple   *status;            /* %s */
	lf_strftime *time;              /* %t, %{format}t */
	lf_fractime *time_frac;         /* %{UNIT}t */
	lf_rtime    *time_taken;        /* %D, %T, %T{UNIT} */
	lf_simple   *remote_user;       /* %u */
	lf_simple   *url_path;          /* %U */
	lf_bool     *server_name;       /* %v, %V */
	lf_simple   *conn_status;       /* %X */
	lf_simple   *bytes_recv;        /* %I */
	lf_simple   *bytes_sent;        /* %O */
	lf_simple   *bytes_xfer;        /* %S */
	lf_name     *req_trailer;       /* %{VARNAME}^ti */
	lf_name     *resp_trailer;      /* %{VARNAME}^to */
};

int
lf_parse(struct lf_config *conf, const char *fmt,
	struct lf_err *ep);

/*
 * Popular log formats, for your convenience.
 */
#define LF_CLF   "%h %l %u %t \"%r\" %>s %b"
#define LF_VHLF  "%v %h %l %u %t \"%r\" %>s %b"
#define LF_NSCA  "%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-agent}i\""
#define LF_RLF   "%{Referer}i -> %U"
#define LF_ALF   "%{User-agent}i"

#endif
