/*
 * Copyright 2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <lf/lf.h>

const char *
lf_strerror(enum lf_errno e)
{
	switch (e) {
	case LF_ERR_MISSING_CLOSING_BRACE:   return "Missing closing brace";
	case LF_ERR_MISSING_DIRECTIVE:       return "Missing directive";
	case LF_ERR_MISSING_ESCAPE:          return "Missing escape";
	case LF_ERR_MISSING_NAME:            return "Missing name";

	case LF_ERR_UNRECOGNISED_DIRECTIVE:  return "Unrecognised directive";
	case LF_ERR_UNRECOGNISED_ESCAPE:     return "Unrecognised escape";
	case LF_ERR_UNRECOGNISED_IP_TYPE:    return "Unrecognised ip type";
	case LF_ERR_UNRECOGNISED_RTIME_UNIT: return "Unrecognised rtime unit";
	case LF_ERR_UNRECOGNISED_PORT_TYPE:  return "Unrecognised port type";
	case LF_ERR_UNRECOGNISED_ID_TYPE:    return "Unrecognised id type";

	case LF_ERR_NAME_OVERFLOW:           return "Name overflow";
	case LF_ERR_STATUS_OVERFLOW:         return "Status overflow";
	case LF_ERR_TOO_MANY_STATUSES:       return "Too many statuses";
	case LF_ERR_TOO_MANY_REDIRECT_FLAGS: return "Too many redirect flags";
	case LF_ERR_NAME_ALREADY_SET:        return "Name already set";
	case LF_ERR_EMPTY_NAME:              return "Empty name";
	case LF_ERR_UNWANTED_NAME:           return "Unwanted name";

	case LF_ERR_HOOK:                    return "Hook error";

	default:
		return "?";
	}
}

