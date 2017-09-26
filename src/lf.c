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
nameeq(const char *b, const char *e, const char *s)
{
	assert(b != NULL);
	assert(e != NULL);
	assert(s != NULL);

	return strncmp(s, b, e - b) == 0 && s[e - b] == '\0';
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

int
lf_parse(struct lf_config *conf, const char *fmt,
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
			const char *b;
			const char *e;
		} name;

		name.b = NULL;
		name.e = NULL;

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
			case 't':  r = conf->literal('\t'); break;
			case 'n':  r = conf->literal('\n'); break;
			case '\'': r = conf->literal('\''); break;
			case '\"': r = conf->literal('\"'); break;
			case '\\': r = conf->literal('\\'); break;

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

				fprintf(stderr, "status: %u\n", u);

				status_storage[pred.count] = u;
				pred.count++;

			} while (*p == ',' && p++);

			if (pred.count > 0) {
				qsort(pred.status, pred.count, sizeof *pred.status,	uintcmp);
			}

			if (*p == '<' || *p == '>') {
				redirectp = p;
				p++;
			} else {
				redirectp = NULL;
			}

			if (*p == '{') {
				const char *e;

				p++;

				e = p + strcspn(p, "}");
				if (*e != '}') {
					ERR(p, e, MISSING_CLOSING_BRACE);
				}

				if (e == p) {
					ERR(p - 1, e + 1, EMPTY_NAME);
				}

				if (name.b != NULL) {
					ERR(p, e, NAME_ALREADY_SET);
				}

				/* TODO: maybe for appropriate statuses only */
				fprintf(stderr, "name: '%.*s'\n", (int) (e - p), p);

				name.b = p;
				name.e = e;

				p = e;

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

			fprintf(stderr, "redirect: %d\n", redirect);

			switch (*p) {
			case 't':
				/*
				 * LogFormat:
				 * "Time the request was received, in the format
				 * [18/Sep/2011:19:18:28 -0400]"
				 */
				if (name.b == NULL) {
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
				if (name.b == NULL) {
					ERR(p - 1, p + 1, MISSING_NAME);
				}

				if ((size_t) (name.e - name.b) > conf->bufsz - 1) {
					ERR(p - 1, p + 1, NAME_OVERFLOW);
				}

				memcpy(conf->buf, name.b, name.e - name.b);
				conf->buf[name.e - name.b] = '\0';

				break;

			default:
				;
			}

/* TODO: store output stuff; neg, status_storage etc */

			switch (*p) {
			case '\0':
				ERR(p - 1, p, MISSING_DIRECTIVE);

			case '%':
				/* TODO: is a status list permitted here? i don't see why not */
				r = conf->literal('%');
				break;

			case 'A': r = conf->ip(&pred, LF_IP_LOCAL);         break;
			case 'B': r = conf->resp_size(&pred);               break;
			case 'b': r = conf->resp_size_clf(&pred);           break;
			case 'C': r = conf->req_cookie(&pred, conf->buf);   break;
			case 'D': r = conf->time_taken(&pred, LF_RTIME_US); break;
			case 'e': r = conf->env_var(&pred, conf->buf);      break;
			case 'f': r = conf->filename(&pred);                break;
			case 'h': r = conf->remote_hostname(&pred, conf->hostname_lookups); break;
			case 'H': r = conf->req_protocol(&pred);            break;
			case 'i': r = conf->req_header(&pred, conf->buf);   break;
			case 'k': r = conf->keepalive_reqs(&pred);          break;
			case 'l': r = conf->remote_logname(&pred);          break;
			case 'L': r = conf->req_logid(&pred);               break;
			case 'm': r = conf->req_method(&pred);              break;
			case 'n': r = conf->note(&pred, conf->buf);         break;
			case 'o': r = conf->reply_header(&pred, conf->buf); break;

			case 'a':
				if (name.b == NULL) {
					r = conf->ip(&pred, LF_IP_CLIENT);
				} else if (nameeq(name.b, name.e, "c")) {
					r = conf->ip(&pred, LF_IP_PEER);
				} else {
					ERR(name.b, name.e, UNRECOGNISED_IP_TYPE);
				}
				break;

			case 'p':
				if (name.b == NULL) {
					r = conf->server_port(&pred, LF_PORT_CANONICAL);
				} else if (nameeq(name.b, name.e, "canonical")) {
					r = conf->ip(&pred, LF_PORT_CANONICAL);
				} else if (nameeq(name.b, name.e, "local")) {
					r = conf->ip(&pred, LF_PORT_LOCAL);
				} else if (nameeq(name.b, name.e, "remote")) {
					r = conf->ip(&pred, LF_PORT_REMOTE);
				} else {
					ERR(name.b, name.e, UNRECOGNISED_PORT_TYPE);
				}
				break;

			case 'P':
				if (name.b == NULL) {
					r = conf->server_port(&pred, LF_ID_PID);
				} else if (nameeq(name.b, name.e, "pid")) {
					r = conf->ip(&pred, LF_ID_PID);
				} else if (nameeq(name.b, name.e, "tid")) {
					r = conf->ip(&pred, LF_ID_TID);
				} else if (nameeq(name.b, name.e, "hextid")) {
					r = conf->ip(&pred, LF_ID_HEXTID);
				} else {
					ERR(name.b, name.e, UNRECOGNISED_ID_TYPE);
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

				fprintf(stderr, "when: %d\n", when);

				/*
				 * LogFormat:
				 * "In addition to the formats supported by strftime(3),
				 * the following format tokens are supported [...]
				 * These tokens can not be combined with each other
				 * or strftime(3) formatting in the same format string.
				 */

				if (0 == strcmp(fmt, "sec")) {
					r = conf->time_frac(&pred, when, LF_RTIME_S);
				} else if (0 == strcmp(fmt, "msec")) {
					r = conf->time_frac(&pred, when, LF_RTIME_MS);
				} else if (0 == strcmp(fmt, "usec")) {
					r = conf->time_frac(&pred, when, LF_RTIME_US);
				} else if (0 == strcmp(fmt, "msec_frac")) {
					r = conf->time_frac(&pred, when, LF_RTIME_MS_FRAC);
				} else if (0 == strcmp(fmt, "usec_frac")) {
					r = conf->time_frac(&pred, when, LF_RTIME_US_FRAC);
				} else {
					r = conf->time(&pred, when, fmt);
				}

				break;
			}

			case 'T':
				if (name.b == NULL) {
					r = conf->time_taken(&pred, LF_RTIME_S);
				} else if (nameeq(name.b, name.e, "ms")) {
					r = conf->time_taken(&pred, LF_RTIME_MS);
				} else if (nameeq(name.b, name.e, "us")) {
					r = conf->time_taken(&pred, LF_RTIME_US);
				} else if (nameeq(name.b, name.e, "s")) {
					r = conf->time_taken(&pred, LF_RTIME_S);
				} else {
					ERR(name.b, name.e, UNRECOGNISED_RTIME_UNIT);
				}
				break;

			case 'u': r = conf->remote_user(&pred);    break;
			case 'U': r = conf->url_path(&pred);       break;
			case 'v': r = conf->server_name(&pred, 1); break;
			case 'V': r = conf->server_name(&pred, conf->use_canonical_name); break;
			case 'X': r = conf->conn_status(&pred);    break;
			case 'I': r = conf->bytes_recv(&pred);     break;
			case 'O': r = conf->bytes_sent(&pred);     break;
			case 'S': r = conf->bytes_xfer(&pred);     break;

			case '^':
				p++;

				if (*p != 't') {
					ERR(p - 2, p, UNRECOGNISED_DIRECTIVE);
				}

				p++;

				switch (*p) {
				case 'i': r = conf->req_trailer (&pred, conf->buf); break;
				case 'o': r = conf->resp_trailer(&pred, conf->buf); break;

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
			r = conf->literal(*p);
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

