/*
 * Copyright 2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <lf/lf.h>

#define MAX_STATUS 0xffffU /* minimum UINT_MAX */

#define ERR(code)         \
	*e = LF_ERR_ ## code; \
	return 0;

struct errstuff {
	const char *toomanyredirect;
	const char *percent;
	const char *endofstatuslist;
	const char *openingbrace;
};

struct txt {
	const char *p;
	size_t n;
};

static int
uintcmp(const void *a, const void *b)
{
	assert(a != NULL);
	assert(b != NULL);

	if (* (unsigned *) a < * (unsigned *) b) {
		return -1;
	}

	if (* (unsigned *) a > * (unsigned *) b) {
		return +1;
	}

	return 0;
}

static int
nameeq(const char *p, size_t n, const char *s)
{
	assert(p != NULL);
	assert(s != NULL);

	if (strlen(s) != n) {
		return 0;
	}

	return 0 == memcmp(p, s, n);
}

static unsigned
parse_status(const char *p, const char **e)
{
	unsigned u;

	assert(p != NULL);

	u = 0;

	while (*p != '\0' && isdigit((unsigned char) *p)) {
		if (u >= MAX_STATUS / 10U) {
			u = 0;
			break;
		}

		u *= 10;
		u += *p - '0';
		p++;
	}

	*e = p;

	return u;
}

static void
uniq(unsigned a[], size_t *count)
{
	size_t i, j;

	assert(count != NULL);

	if (*count <= 1) {
		return;
	}

	j = 0;

	for (i = 1; i < *count; i++) {
		if (a[j] == a[i]) {
			continue;
		}

		j++;
		a[j] = a[i];
	}

	*count = j + 1;
}

static int
notcustom(struct lf_config *conf, void *opaque,
	const char **p, const struct lf_pred *pred, enum lf_redirect redirect,
	const struct txt *name, enum lf_errno *e)
{
	const char *fmt;
	char buf[128]; /* arbitrary limit */

	assert(conf != NULL);
	assert(p != NULL && *p != NULL);
	assert(pred != NULL);
	assert(name != NULL);
	assert(e != NULL);

	switch (**p) {
	case 't':
		/*
		 * LogFormat:
		 * "Time the request was received, in the format
		 * [18/Sep/2011:19:18:28 -0400]"
		 */
		if (name->p == NULL) {
			fmt = "[%d/%b/%Y:%T %z]";
			break;
		}

		fmt = buf;

		/* fallthrough */

	case 'C':
	case 'e':
	case 'i':
	case 'n':
	case 'o':
	case '^':
		if (name->p == NULL) {
			ERR(MISSING_NAME);
		}

		/* XXX: pass pointer and length instead */
		if (name->n > (sizeof buf) - 1) {
			ERR(NAME_OVERFLOW);
		}

		memcpy(buf, name->p, name->n);
		buf[name->n] = '\0';

		break;

	case 'a':
	case 'p':
	case 'P':
	case 'T':
		break;

	default:
		if (!isalpha((unsigned char) **p)) {
			break;
		}

		if (name->p != NULL) {
			ERR(UNWANTED_NAME);
		}

		break;
	}

	switch (**p) {
	case '%':
		/* TODO: is a status list permitted here? i don't see why not */
		return conf->literal(opaque, '%');

	case 'A': return conf->ip(opaque, pred, redirect, LF_IP_LOCAL);
	case 'B': return conf->resp_size(opaque, pred, redirect);
	case 'b': return conf->resp_size_clf(opaque, pred, redirect);
	case 'C': return conf->req_cookie(opaque, pred, redirect, buf);
	case 'D': return conf->time_taken(opaque, pred, redirect, LF_RTIME_US);
	case 'e': return conf->env_var(opaque, pred, redirect, buf);
	case 'f': return conf->filename(opaque, pred, redirect);
	case 'h': return conf->remote_hostname(opaque, pred, redirect, conf->hostname_lookups);
	case 'H': return conf->req_protocol(opaque, pred, redirect);
	case 'i': return conf->req_header(opaque, pred, redirect, buf);
	case 'k': return conf->keepalive_reqs(opaque, pred, redirect);
	case 'l': return conf->remote_logname(opaque, pred, redirect);
	case 'L': return conf->req_logid(opaque, pred, redirect);
	case 'm': return conf->req_method(opaque, pred, redirect);
	case 'n': return conf->note(opaque, pred, redirect, buf);
	case 'o': return conf->reply_header(opaque, pred, redirect, buf);

	case 'a':
		if (name->p == NULL) {
			return conf->ip(opaque, pred, redirect, LF_IP_CLIENT);
		} else if (nameeq(name->p, name->n, "c")) {
			return conf->ip(opaque, pred, redirect, LF_IP_PEER);
		} else {
			ERR(UNRECOGNISED_IP_TYPE);
		}

	case 'p':
		if (name->p == NULL) {
			return conf->server_port(opaque, pred, redirect, LF_PORT_CANONICAL);
		} else if (nameeq(name->p, name->n, "canonical")) {
			return conf->ip(opaque, pred, redirect, LF_PORT_CANONICAL);
		} else if (nameeq(name->p, name->n, "local")) {
			return conf->ip(opaque, pred, redirect, LF_PORT_LOCAL);
		} else if (nameeq(name->p, name->n, "remote")) {
			return conf->ip(opaque, pred, redirect, LF_PORT_REMOTE);
		} else {
			ERR(UNRECOGNISED_PORT_TYPE);
		}

	case 'P':
		if (name->p == NULL) {
			return conf->server_port(opaque, pred, redirect, LF_ID_PID);
		} else if (nameeq(name->p, name->n, "pid")) {
			return conf->ip(opaque, pred, redirect, LF_ID_PID);
		} else if (nameeq(name->p, name->n, "tid")) {
			return conf->ip(opaque, pred, redirect, LF_ID_TID);
		} else if (nameeq(name->p, name->n, "hextid")) {
			return conf->ip(opaque, pred, redirect, LF_ID_HEXTID);
		} else {
			ERR(UNRECOGNISED_ID_TYPE);
		}

	case 't': {
		enum lf_when when;

		/*
		 * LogFormat:
		 * "If the format starts with begin: (default) the time
		 * is taken at the beginning of the req processing.
		 * If it starts with end: it is the time when the log entry
		 * gets written, close to the end of the req processing."
		 */

		if (0 == strncmp(fmt, "begin:", strlen("begin:"))) {
			when = LF_WHEN_BEGIN;
			fmt += strlen("begin:");
		} else if (0 == strncmp(fmt, "end:", strlen("end:"))) {
			when = LF_WHEN_END;
			fmt += strlen("end:");
		} else {
			when = LF_WHEN_BEGIN;
		}

		/*
		 * LogFormat:
		 * "In addition to the formats supported by strftime(3),
		 * the following format tokens are supported [...]
		 * These tokens can not be combined with each other
		 * or strftime(3) formatting in the same format string.
		 */

		if (0 == strcmp(fmt, "sec")) {
			return conf->time_frac(opaque, pred, redirect, when, LF_RTIME_S);
		} else if (0 == strcmp(fmt, "msec")) {
			return conf->time_frac(opaque, pred, redirect, when, LF_RTIME_MS);
		} else if (0 == strcmp(fmt, "usec")) {
			return conf->time_frac(opaque, pred, redirect, when, LF_RTIME_US);
		} else if (0 == strcmp(fmt, "msec_frac")) {
			return conf->time_frac(opaque, pred, redirect, when, LF_RTIME_MS_FRAC);
		} else if (0 == strcmp(fmt, "usec_frac")) {
			return conf->time_frac(opaque, pred, redirect, when, LF_RTIME_US_FRAC);
		} else {
			return conf->time(opaque, pred, redirect, when, fmt);
		}
	}

	case 'T':
		if (name->p == NULL) {
			return conf->time_taken(opaque, pred, redirect, LF_RTIME_S);
		} else if (nameeq(name->p, name->n, "ms")) {
			return conf->time_taken(opaque, pred, redirect, LF_RTIME_MS);
		} else if (nameeq(name->p, name->n, "us")) {
			return conf->time_taken(opaque, pred, redirect, LF_RTIME_US);
		} else if (nameeq(name->p, name->n, "s")) {
			return conf->time_taken(opaque, pred, redirect, LF_RTIME_S);
		} else {
			ERR(UNRECOGNISED_RTIME_UNIT);
		}

	case 'u': return conf->remote_user(opaque, pred, redirect);
	case 'U': return conf->url_path(opaque, pred, redirect);
	case 'v': return conf->server_name(opaque, pred, redirect, 1);
	case 'V': return conf->server_name(opaque, pred, redirect, conf->use_canonical_name);
	case 'X': return conf->conn_status(opaque, pred, redirect);
	case 'I': return conf->bytes_recv(opaque, pred, redirect);
	case 'O': return conf->bytes_sent(opaque, pred, redirect);
	case 'S': return conf->bytes_xfer(opaque, pred, redirect);

	case '^':
		(*p)++;

		if (**p != 't') {
			ERR(UNRECOGNISED_DIRECTIVE);
		}

		(*p)++;

		switch (**p) {
		case 'i': return conf->req_trailer (opaque, pred, redirect, buf);
		case 'o': return conf->resp_trailer(opaque, pred, redirect, buf);

		default:
			ERR(UNRECOGNISED_DIRECTIVE);
		}

	case '\0':
		ERR(MISSING_DIRECTIVE);

	default:
		ERR(UNRECOGNISED_DIRECTIVE);
	}
}

static int
parse_escape(struct lf_config *conf, void *opaque, const char **p,
	enum lf_errno *e)
{
	assert(conf != NULL);
	assert(p != NULL && *p != NULL);
	assert(e != NULL);

	(*p)++;

	/*
	 * LogFormat:
	 * "... and the C-style control characters "\n" and "\t"
	 * to represent new-lines and tabs. Literal quotes and backslashes
	 * should be escaped with backslashes."
	 */

	assert(conf->literal != NULL);

	switch (**p) {
	case 't':  return conf->literal(opaque, '\t');
	case 'n':  return conf->literal(opaque, '\n');
	case '\'': return conf->literal(opaque, '\'');
	case '\"': return conf->literal(opaque, '\"');
	case '\\': return conf->literal(opaque, '\\');

	case '\0':
		ERR(MISSING_ESCAPE);

	default:
		ERR(UNRECOGNISED_ESCAPE);
	}
}

static int
parse_directive(struct lf_config *conf, void *opaque, const char **p,
	enum lf_errno *e, struct errstuff *errstuff)
{
	const char *redirectp;
	unsigned status[128]; /* arbitrary limit */
	struct txt name;
	struct lf_pred pred;
	enum lf_redirect redirect;
	int r;

	assert(conf != NULL);
	assert(p != NULL && *p != NULL);
	assert(e != NULL);
	assert(errstuff != NULL);

	name.p = NULL;

	pred.neg    = 0;
	pred.count  = 0;
	pred.status = status;

	errstuff->percent = *p;

	(*p)++;

	/*
	 * LogFormat:
	 * "Particular items can be restricted to print only for resps
	 * with specific HTTP status codes by placing a comma-separated list
	 * of status codes immediately following the "%". The status code
	 * list may be preceded by a "!" to indicate negation.
	 */

	if (**p == '!') {
		pred.neg = 1;
		(*p)++;
	}

	do {
		const char *end;
		unsigned u;

		/* skip comma-separated list of digits */
		u = parse_status(*p, &end);
		if (u == 0 && end != *p) {
			ERR(STATUS_OVERFLOW);
		}

		if (u == 0) {
			break;
		}

		*p = end;

		errstuff->endofstatuslist = end;

		if (pred.count == sizeof status / sizeof *status) {
			ERR(TOO_MANY_STATUSES);
		}

		status[pred.count] = u;
		pred.count++;

	} while (**p == ',' && (*p)++);

	if (pred.count > 0) {
		qsort(pred.status, pred.count, sizeof *pred.status,	uintcmp);
	}

	uniq(pred.status, &pred.count);

	if (**p == '<' || **p == '>') {
		redirectp = *p;
		(*p)++;
	} else {
		redirectp = NULL;
	}

	if (**p == '{') {
		size_t n;

		errstuff->openingbrace = *p;

		(*p)++;

		n = strcspn(*p, "}");
		if ((*p)[n] != '}') {
			ERR(MISSING_CLOSING_BRACE);
		}

		if (n == 0) {
			ERR(EMPTY_NAME);
		}

		assert(name.p == NULL);

		name.p = *p;
		name.n = n;

		(*p) += n;

		(*p)++;
	}

	/*
	 * LogFormat:
	 * "By default, the % directives %s, %U, %T, %D, and %r
	 * look at the original request while all others look at
	 * the final request."
	 *
	 * There is currently no way to specify equivalent defaults
	 * for custom directives.
	 */
	switch (**p) {
	case 's':
	case 'U':
	case 'T':
	case 'D':
	case 'r': redirect = LF_REDIRECT_ORIG;  break;
	default:  redirect = LF_REDIRECT_FINAL; break;
	}

	if (redirectp != NULL) {
		switch (*redirectp) {
		case '<': redirect = LF_REDIRECT_ORIG;  break;
		case '>': redirect = LF_REDIRECT_FINAL; break;

		default:
			assert(!"unreached");
		}

		errstuff->toomanyredirect = redirectp;

		redirectp++;

		if (*redirectp == '<' || *redirectp == '>') {
			ERR(TOO_MANY_REDIRECT_FLAGS);
		}
	}

	/*
	 * The custom callback decides if the character is relevant.
	 * Note handling for non-alpha characters falls through to
	 * the non-custom directives below, which will then error out.
	 */

	if (conf->override != NULL && isalpha((unsigned char) **p) && strchr(conf->override, **p)) {
		assert(conf->custom != NULL);

		r = conf->custom(conf, opaque, **p, &pred, redirect, name.p, name.n, e);
	} else {
		r = notcustom(conf, opaque, p, &pred, redirect, &name, e);
	}

	return r;
}

int
lf_parse(struct lf_config *conf, void *opaque, const char *fmt,
	struct lf_err *ep)
{
	struct errstuff errstuff;
	enum lf_errno e;
	const char *p;

	assert(conf != NULL);
	assert(ep != NULL);

	for (p = fmt; *p != '\0'; p++) {
		int r;

		errstuff.toomanyredirect = NULL;
		errstuff.percent         = NULL;
		errstuff.endofstatuslist = NULL;
		errstuff.openingbrace    = NULL;

		e = LF_ERR_ERRNO;

		switch (*p) {
		case '\\':
			r = parse_escape(conf, opaque, &p, &e);
			break;

		case '%':
			r = parse_directive(conf, opaque, &p, &e, &errstuff);
			break;

		default:
			r = conf->literal(opaque, *p);
			break;
		}

		if (!r) {
			goto error;
		}
	}

	return 1;

error:

	if (ep != NULL) {
		const char *xp;
		size_t xn;

		/*
		 * Errors from errno are expected to typically come from hooks,
		 * (e.g. some error on printing output). These don't pertain to
		 * any particular input in the fmt string, but we point to where
		 * the cursor is anyway.
		 *
		 * LF_ERR_ERRNO can also be produced by a custom directive,
		 * where the error may or may not be directly caused by the input.
		 * We don't know the difference here.
		 */
		if (e == LF_ERR_ERRNO) {
			ep->errnum = e;
			ep->p = p;
			ep->n = 0;

			return 0;
		}

		/*
		 * Henceforth we know .percent is set for both errors produced by
		 * custom directives (because .custom is only called after '%'
		 * is parsed), and errors about other things (e.g. pointing at
		 * escapes) do not expect .percent to be set.
		 */

		switch (e) {
		case LF_ERR_MISSING_CLOSING_BRACE:
			if (errstuff.openingbrace == NULL) {
				goto percent;
			}
			xp = errstuff.openingbrace;
			xn = 1;
			break;

		case LF_ERR_MISSING_ESCAPE:
		case LF_ERR_UNRECOGNISED_ESCAPE:
			xp = p - 1;
			xn = 1 + (*p != '\0');
			break;

		case LF_ERR_TOO_MANY_STATUSES:
			if (errstuff.endofstatuslist == NULL) {
				goto percent;
			}
			xp = p;
			xn = errstuff.endofstatuslist - p;
			break;

		case LF_ERR_STATUS_OVERFLOW:
			xp = p;
			xn = strspn(p, "0123456789");
			break;

		case LF_ERR_TOO_MANY_REDIRECT_FLAGS:
			if (errstuff.toomanyredirect == NULL) {
				goto percent;
			}
			xp = errstuff.toomanyredirect;
			xn = strspn(errstuff.toomanyredirect, "<>");
			break;

		case LF_ERR_NAME_OVERFLOW:
		case LF_ERR_UNRECOGNISED_ID_TYPE:
		case LF_ERR_UNRECOGNISED_IP_TYPE:
		case LF_ERR_UNRECOGNISED_PORT_TYPE:
		case LF_ERR_UNRECOGNISED_RTIME_UNIT:
			if (errstuff.openingbrace == NULL) {
				goto percent;
			}
			xp = errstuff.openingbrace + 1;
			xn = strcspn(xp, "}");
			/* TODO: cut down xn to (sizeof buf) - 1 here */
			break;

		case LF_ERR_EMPTY_NAME:
		case LF_ERR_UNWANTED_NAME:
			if (errstuff.openingbrace == NULL) {
				goto percent;
			}
			xp = errstuff.openingbrace;
			xn = strcspn(xp, "}") + 1;
			break;

percent:

		case LF_ERR_UNSUPPORTED:
		case LF_ERR_MISSING_NAME:
		case LF_ERR_UNRECOGNISED_DIRECTIVE:
			assert(errstuff.percent != NULL);
			xp = errstuff.percent;
			xn = p - errstuff.percent + 1;
			break;

		case LF_ERR_MISSING_DIRECTIVE:
			assert(errstuff.percent != NULL);
			xp = errstuff.percent;
			xn = p - errstuff.percent;
			break;

		default:
			assert(!"unreached");
		}

		ep->errnum = e;

		ep->p = xp;
		ep->n = xn;
	}

	return 0;
}

