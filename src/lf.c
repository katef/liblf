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

#define ERR(xp, xe, code) \
	if (ep != NULL) { \
		ep->errnum = LF_ERR_ ## code; \
		ep->p = (xp); \
		ep->e = (xe); \
	} \
	goto error;

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

	while (p != '\0' && isdigit((unsigned char) *p)) {
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

int
lf_parse(struct lf_config *conf, void *opaque, const char *fmt,
	struct lf_err *ep)
{
	const char *p;

	assert(conf != NULL);
	assert(ep != NULL);

	for (p = fmt; *p != '\0'; p++) {
		/* XXX: expect user to point to storage instead */
		unsigned status_storage[128]; /* arbitrary limit */

		struct lf_pred pred;
		enum lf_redirect redirect;
		const char *fmt;
		int r;

		struct {
			const char *p;
			size_t n;
		} name;

		name.p = NULL;

		pred.neg    = 0;
		pred.count  = 0;
		pred.status = status_storage;

		switch (*p) {
			const char *redirectp;

		case '\\':
			p++;

			/*
			 * LogFormat:
			 * "... and the C-style control characters "\n" and "\t"
			 * to represent new-lines and tabs. Literal quotes and backslashes
			 * should be escaped with backslashes."
			 */

			assert(conf->literal != NULL);

			switch (*p) {
			case 't':  r = conf->literal(opaque, '\t'); break;
			case 'n':  r = conf->literal(opaque, '\n'); break;
			case '\'': r = conf->literal(opaque, '\''); break;
			case '\"': r = conf->literal(opaque, '\"'); break;
			case '\\': r = conf->literal(opaque, '\\'); break;

			case '\0':
				ERR(p - 1, p, MISSING_ESCAPE);

			default:
				ERR(p - 1, p, UNRECOGNISED_ESCAPE);
			}

			break;

		case '%':
			p++;

			/*
			 * LogFormat:
			 * "Particular items can be restricted to print only for resps
			 * with specific HTTP status codes by placing a comma-separated list
			 * of status codes immediately following the "%". The status code
			 * list may be preceded by a "!" to indicate negation.
			 */

			if (*p == '!') {
				pred.neg = 1;
				p++;
			}

			do {
				const char *e;
				unsigned u;

				/* skip comma-separated list of digits */
				u = parse_status(p, &e);
				if (u == 0 && e != p) {
					ERR(p, e, STATUS_OVERFLOW);
				}

				if (u == 0) {
					break;
				}

				p = e;

				if (pred.count == sizeof status_storage / sizeof *status_storage) {
					ERR(p, e, TOO_MANY_STATUSES);
				}

				status_storage[pred.count] = u;
				pred.count++;

			} while (*p == ',' && p++);

			if (pred.count > 0) {
				qsort(pred.status, pred.count, sizeof *pred.status,	uintcmp);
			}

			uniq(pred.status, &pred.count);

			if (*p == '<' || *p == '>') {
				redirectp = p;
				p++;
			} else {
				redirectp = NULL;
			}

			if (*p == '{') {
				size_t n;

				p++;

				n = strcspn(p, "}");
				if (p[n] != '}') {
					ERR(p, p + n, MISSING_CLOSING_BRACE);
				}

				if (n == 0) {
					ERR(p - 1, p + n + 1, EMPTY_NAME);
				}

				if (name.p != NULL) {
					ERR(p, p + n, NAME_ALREADY_SET);
				}

				name.p = p;
				name.n = n;

				p += n;

				p++;
			}

			/*
			 * LogFormat:
			 * "By default, the % directives %s, %U, %T, %D, and %r
			 * look at the original request while all others look at
			 * the final request."
			 */
			switch (*p) {
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

				redirectp++;

				if (*redirectp == '<' || *redirectp == '>') {
					ERR(redirectp - 1, redirectp, TOO_MANY_REDIRECT_FLAGS);
				}
			}

			switch (*p) {
			case 't':
				/*
				 * LogFormat:
				 * "Time the request was received, in the format
				 * [18/Sep/2011:19:18:28 -0400]"
				 */
				if (name.p == NULL) {
					fmt = "[%d/%b/%Y:%T %z]";
					break;
				}

				fmt = conf->buf;

				/* fallthrough */

			case 'C':
			case 'e':
			case 'i':
			case 'n':
			case 'o':
			case '^':
				if (name.p == NULL) {
					ERR(p - 1, p + 1, MISSING_NAME);
				}

				if (name.n > conf->bufsz - 1) {
					ERR(p - 1, p + 1, NAME_OVERFLOW);
				}

				memcpy(conf->buf, name.p, name.n);
				conf->buf[name.n] = '\0';

				break;

			default:
				;
			}

			switch (*p) {
			case '\0':
				ERR(p - 1, p, MISSING_DIRECTIVE);

			case '%':
				/* TODO: is a status list permitted here? i don't see why not */
				r = conf->literal(opaque, '%');
				break;

			case 'A': r = conf->ip(opaque, &pred, redirect, LF_IP_LOCAL);         break;
			case 'B': r = conf->resp_size(opaque, &pred, redirect);               break;
			case 'b': r = conf->resp_size_clf(opaque, &pred, redirect);           break;
			case 'C': r = conf->req_cookie(opaque, &pred, redirect, conf->buf);   break;
			case 'D': r = conf->time_taken(opaque, &pred, redirect, LF_RTIME_US); break;
			case 'e': r = conf->env_var(opaque, &pred, redirect, conf->buf);      break;
			case 'f': r = conf->filename(opaque, &pred, redirect);                break;
			case 'h': r = conf->remote_hostname(opaque, &pred, redirect, conf->hostname_lookups); break;
			case 'H': r = conf->req_protocol(opaque, &pred, redirect);            break;
			case 'i': r = conf->req_header(opaque, &pred, redirect, conf->buf);   break;
			case 'k': r = conf->keepalive_reqs(opaque, &pred, redirect);          break;
			case 'l': r = conf->remote_logname(opaque, &pred, redirect);          break;
			case 'L': r = conf->req_logid(opaque, &pred, redirect);               break;
			case 'm': r = conf->req_method(opaque, &pred, redirect);              break;
			case 'n': r = conf->note(opaque, &pred, redirect, conf->buf);         break;
			case 'o': r = conf->reply_header(opaque, &pred, redirect, conf->buf); break;

			case 'a':
				if (name.p == NULL) {
					r = conf->ip(opaque, &pred, redirect, LF_IP_CLIENT);
				} else if (nameeq(name.p, name.n, "c")) {
					r = conf->ip(opaque, &pred, redirect, LF_IP_PEER);
				} else {
					ERR(name.p, name.p + name.n, UNRECOGNISED_IP_TYPE);
				}
				break;

			case 'p':
				if (name.p == NULL) {
					r = conf->server_port(opaque, &pred, redirect, LF_PORT_CANONICAL);
				} else if (nameeq(name.p, name.n, "canonical")) {
					r = conf->ip(opaque, &pred, redirect, LF_PORT_CANONICAL);
				} else if (nameeq(name.p, name.n, "local")) {
					r = conf->ip(opaque, &pred, redirect, LF_PORT_LOCAL);
				} else if (nameeq(name.p, name.n, "remote")) {
					r = conf->ip(opaque, &pred, redirect, LF_PORT_REMOTE);
				} else {
					ERR(name.p, name.p + name.n, UNRECOGNISED_PORT_TYPE);
				}
				break;

			case 'P':
				if (name.p == NULL) {
					r = conf->server_port(opaque, &pred, redirect, LF_ID_PID);
				} else if (nameeq(name.p, name.n, "pid")) {
					r = conf->ip(opaque, &pred, redirect, LF_ID_PID);
				} else if (nameeq(name.p, name.n, "tid")) {
					r = conf->ip(opaque, &pred, redirect, LF_ID_TID);
				} else if (nameeq(name.p, name.n, "hextid")) {
					r = conf->ip(opaque, &pred, redirect, LF_ID_HEXTID);
				} else {
					ERR(name.p, name.p + name.n, UNRECOGNISED_ID_TYPE);
				}
				break;

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
					r = conf->time_frac(opaque, &pred, redirect, when, LF_RTIME_S);
				} else if (0 == strcmp(fmt, "msec")) {
					r = conf->time_frac(opaque, &pred, redirect, when, LF_RTIME_MS);
				} else if (0 == strcmp(fmt, "usec")) {
					r = conf->time_frac(opaque, &pred, redirect, when, LF_RTIME_US);
				} else if (0 == strcmp(fmt, "msec_frac")) {
					r = conf->time_frac(opaque, &pred, redirect, when, LF_RTIME_MS_FRAC);
				} else if (0 == strcmp(fmt, "usec_frac")) {
					r = conf->time_frac(opaque, &pred, redirect, when, LF_RTIME_US_FRAC);
				} else {
					r = conf->time(opaque, &pred, redirect, when, fmt);
				}

				break;
			}

			case 'T':
				if (name.p == NULL) {
					r = conf->time_taken(opaque, &pred, redirect, LF_RTIME_S);
				} else if (nameeq(name.p, name.n, "ms")) {
					r = conf->time_taken(opaque, &pred, redirect, LF_RTIME_MS);
				} else if (nameeq(name.p, name.n, "us")) {
					r = conf->time_taken(opaque, &pred, redirect, LF_RTIME_US);
				} else if (nameeq(name.p, name.n, "s")) {
					r = conf->time_taken(opaque, &pred, redirect, LF_RTIME_S);
				} else {
					ERR(name.p, name.p + name.n, UNRECOGNISED_RTIME_UNIT);
				}
				break;

			case 'u': r = conf->remote_user(opaque, &pred, redirect);    break;
			case 'U': r = conf->url_path(opaque, &pred, redirect);       break;
			case 'v': r = conf->server_name(opaque, &pred, redirect, 1); break;
			case 'V': r = conf->server_name(opaque, &pred, redirect, conf->use_canonical_name); break;
			case 'X': r = conf->conn_status(opaque, &pred, redirect);    break;
			case 'I': r = conf->bytes_recv(opaque, &pred, redirect);     break;
			case 'O': r = conf->bytes_sent(opaque, &pred, redirect);     break;
			case 'S': r = conf->bytes_xfer(opaque, &pred, redirect);     break;

			case '^':
				p++;

				if (*p != 't') {
					ERR(p - 2, p, UNRECOGNISED_DIRECTIVE);
				}

				p++;

				switch (*p) {
				case 'i': r = conf->req_trailer (opaque, &pred, redirect, conf->buf); break;
				case 'o': r = conf->resp_trailer(opaque, &pred, redirect, conf->buf); break;

				default:
					ERR(p - 3, p, UNRECOGNISED_DIRECTIVE);
				}

				break;

			default:
				ERR(p - 1, p + 1, UNRECOGNISED_DIRECTIVE);
				goto error;
			}

			break;

		default:
			r = conf->literal(opaque, *p);
			break;
		}

		if (!r) {
			ERR(p - 1, p + 1, HOOK);
		}
	}

	return 1;

error:

	return 0;
}

