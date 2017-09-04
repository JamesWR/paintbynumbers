#include "stdinclude.h"

void dbformat(char *str, char *ostr);

/* convert database return string into a string for header printing */

void dbformat(char *str, char *ostr)
{
	char *pbn_id;
	char *pbn_name;
	char *pbn_style;
	char *pbn_size;
	char *pbn_ctime;
	char *pbn_mtime;
	char *pbn_iterations;
	char *pbn_notes;
	char *pbn_pending;
	char *istr = str;

	/* remove newline */
	while (*str && *str != '\n') str++;
	if (*str) *str++ = 0;
	str = istr;

	pbn_id = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_name = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_style = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_size = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_ctime = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_mtime = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_iterations = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_notes = str;
	while (*str && *str != '\t') str++;
	if (*str) *str++ = 0;
	pbn_pending = str;
	sprintf(ostr, "%s %s %s %s", pbn_size, pbn_ctime, pbn_mtime, pbn_notes);
}
