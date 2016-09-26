/*
 * util.c: common, generic utility functions
 *
 * See COPYING for the license of this software
 *
 * Written by:
 *      Michal Privoznik <mprivozn@redhat.com>
 */

#include <config.h>

#include <stdlib.h>
#include <time.h>

#include "util.h"

/*
 * Private function name:   get_datetime
 * Since version:           0.4.2
 * Description:             Function can be used to get date and time in the `YYYY-mm-dd HH:mm:ss` format, internally used for logging when debug logging is enabled using libvirt_set_logfile() API function.
 * Arguments:               None
 * Returns:                 Date/time string in `YYYY-mm-dd HH:mm:ss` format
 */
char *get_datetime(void)
{
    /* Caution: Function cannot use DPRINTF() macro otherwise the neverending loop will be met! */
    char *outstr = NULL;
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL)
        return NULL;

    outstr = (char *)malloc(32 * sizeof(char));
    if (strftime(outstr, 32, "%Y-%m-%d %H:%M:%S", tmp) == 0)
        return NULL;

    return outstr;
}
