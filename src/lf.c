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

#define ERR(code)                     \
	if (ep != NULL) {                 \
	    ep->errnum = LF_ERR_ ## code; \
	}                                 \
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

int
lf_parse(struct lf_config *conf, void *opaque, const char *fmt,
	struct lf_err *ep)
{
	const char *p;

	struct txt {
		const char *p;
		size_t n;
	};

	struct errstuff {
		const char *toomanyredirect;
		const char *percent;
		const char *endofstatuslist;
		const char *openingbrace;
	} errstuff;

	assert(conf != NULL);
	assert(ep != NULL);

	for (p = fmt; *p != '\0'; p++) {
		unsigned status[128]; /* arbitrary limit */
		char buf[128]; /* arbitrary limit */

		struct txt name;
		struct lf_pred pred;
		enum lf_redirect redirect;
		const char *fmt;
		int r;

		errstuff.toomanyredirect = NULL;
		errstuff.percent         = NULL;
		errstuff.endofstatuslist = NULL;
		errstuff.openingbrace    = NULL;

		name.p = NULL;

		pred.neg    = 0;
		pred.count  = 0;
		pred.status = status;

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
				ERR(MISSING_ESCAPE);

			default:
				ERR(UNRECOGNISED_ESCAPE);
			}

			break;

		case '%':
			errstuff.percent = p;

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
					ERR(STATUS_OVERFLOW);
				}

				if (u == 0) {
					break;
				}

				p = e;

				errstuff.endofstatuslist = e;

				if (pred.count == sizeof status / sizeof *status) {
					ERR(TOO_MANY_STATUSES);
				}

				status[pred.count] = u;
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

				errstuff.openingbrace = p;

				p++;

				n = strcspn(p, "}");
				if (p[n] != '}') {
					ERR(MISSING_CLOSING_BRACE);
				}

				if (n == 0) {
					ERR(EMPTY_NAME);
				}

				assert(name.p == NULL);

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
			 *
			 * There is currently no way to specify equivalent defaults
			 * for custom directives.
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

				errstuff.toomanyredirect = redirectp;

				redirectp++;

				if (*redirectp == '<' || *redirectp == '>') {
					ERR(TOO_MANY_REDIRECT_FLAGS);
				}
			}

			/*
			 * The custom callback decides if the character is relevant.
			 * Note for non-alpha handling falls through to the non-custom
			 * directives below, which will then error out.
			 */

			if (conf->override != NULL && isalpha((unsigned char) *p) && strchr(conf->override, *p)) {
				enum lf_errno e;

				assert(conf->custom != NULL);

				r = conf->custom(conf, opaque, *p, &pred, redirect, name.p, name.n, &e);
				if (!r) {
					/* TODO: combine with ERR() */
					if (ep != NULL) {
						ep->errnum = e;
					}

					goto error;
				}

			} else {

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

					fmt = buf;

					/* fallthrough */

				case 'C':
				case 'e':
				case 'i':
				case 'n':
				case 'o':
				case '^':
					if (name.p == NULL) {
						ERR(MISSING_NAME);
					}

					/* XXX: pass pointer and length instead */
					if (name.n > (sizeof buf) - 1) {
						ERR(NAME_OVERFLOW);
					}

					memcpy(buf, name.p, name.n);
					buf[name.n] = '\0';

					break;

				case 'a':
				case 'p':
				case 'P':
				case 'T':
					break;

				default:
					if (!isalpha((unsigned char) *p)) {
						break;
					}

					if (name.p != NULL) {
						ERR(UNWANTED_NAME);
					}

					break;
				}

				switch (*p) {
				case '%':
					/* TODO: is a status list permitted here? i don't see why not */
					r = conf->literal(opaque, '%');
					break;

				case 'A': r = conf->ip(opaque, &pred, redirect, LF_IP_LOCAL);         break;
				case 'B': r = conf->resp_size(opaque, &pred, redirect);               break;
				case 'b': r = conf->resp_size_clf(opaque, &pred, redirect);           break;
				case 'C': r = conf->req_cookie(opaque, &pred, redirect, buf);         break;
				case 'D': r = conf->time_taken(opaque, &pred, redirect, LF_RTIME_US); break;
				case 'e': r = conf->env_var(opaque, &pred, redirect, buf);            break;
				case 'f': r = conf->filename(opaque, &pred, redirect);                break;
				case 'h': r = conf->remote_hostname(opaque, &pred, redirect, conf->hostname_lookups); break;
				case 'H': r = conf->req_protocol(opaque, &pred, redirect);            break;
				case 'i': r = conf->req_header(opaque, &pred, redirect, buf);         break;
				case 'k': r = conf->keepalive_reqs(opaque, &pred, redirect);          break;
				case 'l': r = conf->remote_logname(opaque, &pred, redirect);          break;
				case 'L': r = conf->req_logid(opaque, &pred, redirect);               break;
				case 'm': r = conf->req_method(opaque, &pred, redirect);              break;
				case 'n': r = conf->note(opaque, &pred, redirect, buf);               break;
				case 'o': r = conf->reply_header(opaque, &pred, redirect, buf);       break;

				case 'a':
					if (name.p == NULL) {
						r = conf->ip(opaque, &pred, redirect, LF_IP_CLIENT);
					} else if (nameeq(name.p, name.n, "c")) {
						r = conf->ip(opaque, &pred, redirect, LF_IP_PEER);
					} else {
						ERR(UNRECOGNISED_IP_TYPE);
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
						ERR(UNRECOGNISED_PORT_TYPE);
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
						ERR(UNRECOGNISED_ID_TYPE);
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
						ERR(UNRECOGNISED_RTIME_UNIT);
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
						ERR(UNRECOGNISED_DIRECTIVE);
					}

					p++;

					switch (*p) {
					case 'i': r = conf->req_trailer (opaque, &pred, redirect, buf); break;
					case 'o': r = conf->resp_trailer(opaque, &pred, redirect, buf); break;

					default:
						ERR(UNRECOGNISED_DIRECTIVE);
					}

					break;

				case '\0':
					ERR(MISSING_DIRECTIVE);

				default:
					ERR(UNRECOGNISED_DIRECTIVE);
					goto error;
				}

				break;

			default:
				r = conf->literal(opaque, *p);
				break;
			}

		}

		if (!r) {
			ERR(HOOK);
		}
	}

	return 1;

error:

	if (ep != NULL) {
		const char *xp;
		size_t xn;

		switch (ep->errnum) {
		case LF_ERR_MISSING_CLOSING_BRACE:   xp = errstuff.openingbrace; xn = 0; break;
		case LF_ERR_MISSING_ESCAPE:          xp = p - 1; xn = 1; break;
		case LF_ERR_UNRECOGNISED_ESCAPE:     xp = p - 1; xn = 2; break;
		case LF_ERR_HOOK:                    xp = p - 1; xn = 2; break; /* XXX: should be whole directive */

		case LF_ERR_TOO_MANY_STATUSES:       xp = p; xn = errstuff.endofstatuslist - p; break; /* XXX: endofstatuslist may be NULL, if this is from a custom callback */
		case LF_ERR_STATUS_OVERFLOW:         xp = p; xn = strspn(p, "023456789"); break;

		case LF_ERR_TOO_MANY_REDIRECT_FLAGS: xp = errstuff.toomanyredirect; xn = strspn(errstuff.toomanyredirect, "<>"); break;

		case LF_ERR_MISSING_DIRECTIVE:       xp = errstuff.percent; xn = p - errstuff.percent; break;
		case LF_ERR_MISSING_NAME:            xp = errstuff.percent; xn = p - errstuff.percent + 1; break;
		case LF_ERR_UNRECOGNISED_DIRECTIVE:  xp = errstuff.percent; xn = p - errstuff.percent + 1; break;

		case LF_ERR_EMPTY_NAME:
			xp = errstuff.openingbrace;
			xn = 2;
			break;

		case LF_ERR_NAME_OVERFLOW:
			xp = errstuff.openingbrace + 1;
			xn = strcspn(xp, "}");
			break;

		case LF_ERR_UNRECOGNISED_ID_TYPE:
		case LF_ERR_UNRECOGNISED_IP_TYPE:
		case LF_ERR_UNRECOGNISED_PORT_TYPE:
		case LF_ERR_UNRECOGNISED_RTIME_UNIT:
			xp = errstuff.openingbrace + 1;
			xn = strcspn(xp, "}");
			break;

		case LF_ERR_UNWANTED_NAME:
			xp = errstuff.openingbrace;
			xn = strcspn(xp, "}") + 1;
			break;

		default:
			assert(!"unreached");
		}

		/* TODO: if we're showing name.p, cut down xn to (sizeof buf) - 1 here */

		/* fall outwards to the entire directive */
		if (xp == NULL) {
			xp = errstuff.percent;
			if (xn != 0) {
				xn = p - errstuff.percent;
			}
		}

		/* XXX: errstuff.percent may be NULL; fall back to xp = p; xn = 0 */

		ep->p = xp;
		ep->n = xn;
	}

	return 0;
	}

