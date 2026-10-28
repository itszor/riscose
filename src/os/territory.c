/* os/territory.c - implementation of the Territory module. */

/* See http://riscose.sf.net/ for terms of distribution, and to
 * pick up a later version of the software.
 *
 * Created by defmod, riscose version 1.01. */

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "monty/monty.h"
#include "types.h"
#include "territory.h"
#include "mem.h"
#include "rom/rom.h"


/* ---- territory_swi_register_extra -------------------------------- */

void territory_swi_register_extra(void)
{
    return;
}

/* ---- xterritory_number ------------------------------------------- */

os_error *xterritory_number(territory_t *territory)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_register ----------------------------------------- */

os_error *xterritory_register(territory_t territory,
    byte *entries,
    void *workspace)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_deregister --------------------------------------- */

os_error *xterritory_deregister(territory_t territory)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_number_to_name ----------------------------------- */

os_error *xterritory_number_to_name(territory_t territory,
    char *name,
    int size)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_exists ------------------------------------------- */

os_error *xterritory_exists(territory_t territory,
    bits *psr)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_alphabet_number_to_name -------------------------- */

os_error *xterritory_alphabet_number_to_name(territory_alphabet_number alphabet,
    char *name,
    int size)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_select_alphabet ---------------------------------- */

os_error *xterritory_select_alphabet(territory_t territory)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_set_time ----------------------------------------- */

os_error *xterritory_set_time(os_date_and_time *date_and_time)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_read_current_time_zone --------------------------- */

os_error *xterritory_read_current_time_zone(char **timezone,
    int *offset)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_convert_time_to_utc_ordinals --------------------- */

os_error *xterritory_convert_time_to_utc_ordinals(os_date_and_time *date_and_time,
    territory_ordinals *ordinals)
{
    uint64_t ts = 0;
    struct tm *tm;
    time_t t;

    if (date_and_time == NULL || ordinals == NULL)
        return ERR_BAD_PARAMETERS();

    for (size_t i = 0; i < sizeof(*date_and_time); i++) {
        ts |= ((uint64_t) ((byte *) date_and_time)[i]) << (8*i);
    }

    ordinals->centisecond = ts % 100;
    t = (ts / 100) - 2208988800L;
    tm = gmtime(&t);
    if (tm == NULL)
        return ERR_BAD_TIME();

    ordinals->second = tm->tm_sec;
    ordinals->minute = tm->tm_min;
    ordinals->hour = tm->tm_hour;
    ordinals->date = tm->tm_mday;
    ordinals->month = tm->tm_mon + 1;
    ordinals->year = tm->tm_year + 1900;
    ordinals->weekday = tm->tm_wday + 1;
    ordinals->yearday = tm->tm_yday + 1;

    return NULL;
}

/* ---- xterritory_read_time_zones ---------------------------------- */

os_error *xterritory_read_time_zones(territory_t territory,
    char **timezone,
    char **dst_timezone,
    int *offset,
    int *dst_offset)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- riscos_format_date_and_time --------------------------------- */

/* Format a 5-byte time (centiseconds since 1900, UTC) as local time, as
 * OS_ConvertDateAndTime and Territory_ConvertDateAndTime do: the format's
 * %-codes are two letters (either case), %Z before one drops its leading
 * zeros, and %% is a %.  *end is left at the terminating null. */

os_error *riscos_format_date_and_time(os_date_and_time *date_and_time,
    char *buffer,
    int size,
    const char *format,
    char **end)
{
    static const char *days[] = { "Sunday", "Monday", "Tuesday",
        "Wednesday", "Thursday", "Friday", "Saturday" };
    static const char *months[] = { "January", "February", "March",
        "April", "May", "June", "July", "August", "September", "October",
        "November", "December" };
    uint64_t ts = 0;
    struct tm *tm;
    time_t t;
    char *out = buffer;

    if (date_and_time == NULL || buffer == NULL || format == NULL)
        return ERR_BAD_PARAMETERS();

    for (size_t i = 0; i < sizeof(*date_and_time); i++) {
        ts |= ((uint64_t) ((byte *) date_and_time)[i]) << (8*i);
    }
    t = (ts / 100) - 2208988800L;
    tm = localtime(&t);
    if (tm == NULL)
        return ERR_BAD_TIME();

    while (*format >= ' ') {
        char field[32];
        const char *f = field;
        int zero = 0, n = -1;

        if (*format != '%') {
            field[0] = *format++;
            field[1] = '\0';
        } else if (format[1] == '%') {
            strcpy(field, "%");
            format += 2;
        } else {
            char code[3];

            format++;
            if (toupper((unsigned char) *format) == 'Z') {
                zero = 1;
                format++;
            }
            if (format[0] < ' ' || format[1] < ' ')
                return ERR_BAD_PARAMETERS();
            code[0] = toupper((unsigned char) format[0]);
            code[1] = toupper((unsigned char) format[1]);
            code[2] = '\0';
            format += 2;

            field[0] = '\0';
            if (!strcmp(code, "CS")) n = ts % 100;
            else if (!strcmp(code, "SE")) n = tm->tm_sec;
            else if (!strcmp(code, "MI")) n = tm->tm_min;
            else if (!strcmp(code, "12")) n = (tm->tm_hour + 11) % 12 + 1;
            else if (!strcmp(code, "24")) n = tm->tm_hour;
            else if (!strcmp(code, "AM") || !strcmp(code, "PM"))
                f = tm->tm_hour < 12 ? "am" : "pm";
            else if (!strcmp(code, "WE")) f = days[tm->tm_wday];
            else if (!strcmp(code, "W3")) snprintf(field, sizeof field, "%.3s", days[tm->tm_wday]);
            else if (!strcmp(code, "WN")) snprintf(field, sizeof field, "%d", tm->tm_wday + 1);
            else if (!strcmp(code, "DY")) n = tm->tm_mday;
            else if (!strcmp(code, "ST")) {
                int d = tm->tm_mday;
                f = (d / 10 == 1) ? "th" : d % 10 == 1 ? "st" :
                    d % 10 == 2 ? "nd" : d % 10 == 3 ? "rd" : "th";
            }
            else if (!strcmp(code, "MO")) f = months[tm->tm_mon];
            else if (!strcmp(code, "M3")) snprintf(field, sizeof field, "%.3s", months[tm->tm_mon]);
            else if (!strcmp(code, "MN")) n = tm->tm_mon + 1;
            else if (!strcmp(code, "CE")) n = (tm->tm_year + 1900) / 100;
            else if (!strcmp(code, "YR")) n = tm->tm_year % 100;
            else if (!strcmp(code, "WK")) strftime(field, sizeof field, "%W", tm);
            else if (!strcmp(code, "DN")) snprintf(field, sizeof field, zero ? "%d" : "%03d", tm->tm_yday + 1);
            else if (!strcmp(code, "TZ")) strftime(field, sizeof field, "%Z", tm);
            else
                return ERR_BAD_PARAMETERS();

            if (n >= 0)
                snprintf(field, sizeof field, zero ? "%d" : "%02d", n);
        }

        if (out - buffer + (int) strlen(f) >= size)
            return ERR_BUFF_OVERFLOW();
        strcpy(out, f);
        out += strlen(f);
    }

    *out = '\0';
    if (end != NULL)
        *end = out;
    return NULL;
}

/* ---- xterritory_convert_date_and_time ---------------------------- */

os_error *xterritory_convert_date_and_time(territory_t territory,
    os_date_and_time *date_and_time,
    char *buffer,
    int size,
    char *format,
    char **end)
{
    return riscos_format_date_and_time(date_and_time, buffer, size, format,
        end);
}

/* ---- xterritory_convert_standard_date_and_time ------------------- */

os_error *xterritory_convert_standard_date_and_time(territory_t territory,
    os_date_and_time *date_and_time,
    char *buffer,
    int size,
    char **end)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_convert_standard_date ---------------------------- */

os_error *xterritory_convert_standard_date(territory_t territory,
    os_date_and_time *date,
    char *buffer,
    int size,
    char **end)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_convert_standard_time ---------------------------- */

os_error *xterritory_convert_standard_time(territory_t territory,
    os_date_and_time *time,
    char *buffer,
    int size,
    char **end)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_convert_time_to_ordinals ------------------------- */

os_error *xterritory_convert_time_to_ordinals(territory_t territory,
    os_date_and_time *date_and_time,
    territory_ordinals *ordinals)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_convert_time_string_to_ordinals ------------------ */

os_error *xterritory_convert_time_string_to_ordinals(territory_t territory,
    territory_time_string_flags format,
    char *t,
    territory_ordinals *ordinals)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_convert_ordinals_to_time ------------------------- */

os_error *xterritory_convert_ordinals_to_time(territory_t territory,
    os_date_and_time *buffer,
    territory_ordinals *ordinals)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_alphabet ----------------------------------------- */

os_error *xterritory_alphabet(territory_t territory,
    territory_alphabet_number *alphabet)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_alphabet_identifier ------------------------------ */

os_error *xterritory_alphabet_identifier(territory_t territory,
    char **alphabet_name)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_select_keyboard_handler -------------------------- */

os_error *xterritory_select_keyboard_handler(territory_t territory)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_write_direction ---------------------------------- */

os_error *xterritory_write_direction(territory_t territory,
    territory_write_flags *flags)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_character_property_table ------------------------- */

os_error *xterritory_character_property_table(territory_t territory,
    territory_character_property property,
    territory_property_table **table)
{
    static territory_property_table *map[10];

    if (property < 0 || property > 10) {
        error("Bad character property\n");
        return NULL;
    }
    if (!map[property]) {
        map[property] = (territory_property_table *) mem_rma_alloc(sizeof(territory_property_table));

        for (int c=0; c<256; c++) {
            int out;
            switch (property) {
                case 0:
                    out = iscntrl(c);
                    break;
                case 1:
                    out = isupper(c);
                    break;
                case 2:
                    out = islower(c);
                    break;
                case 3:
                    out = isalpha(c);
                    break;
                case 4:
                    out = ispunct(c);
                    break;
                case 5:
                    out = isspace(c);
                    break;
                case 6:
                    out = isdigit(c);
                    break;
                case 7:
                    out = isxdigit(c);
                    break;
                case 8:
                    out = isalpha(c) && (c > 127);
                    break;
                case 9:
                    out = 1;
                    break;
                case 10:
                    out = 0;
                    break;
                default:
                    assert(1==0);
                    break;
            }
            map[property]->p[c>>5] |= out ? (1<<(c&0x1f)) : 0;
        }
    }
    *table = map[property];

    return NULL;
}

/* ---- xterritory_lower_case_table --------------------------------- */

os_error *xterritory_lower_case_table(territory_t territory,
    char **table)
{
    static char *lower_case_table = NULL;
    if (!lower_case_table) {
        lower_case_table = mem_rma_alloc(sizeof(char)*256);
        for (int c=0; c<256; c++) {
            lower_case_table[c] = tolower(c);
        }
    }
    *table = lower_case_table;

    return NULL;
}

/* ---- xterritory_upper_case_table --------------------------------- */


os_error *xterritory_upper_case_table(territory_t territory,
    char **table)
{
    static char *upper_case_table = NULL;
    if (!upper_case_table) {
        upper_case_table = mem_rma_alloc(sizeof(char)*256);
        for (int c=0; c<256; c++) {
            upper_case_table[c] = toupper(c);
        }
    }
    *table = upper_case_table;

    return NULL;
}

/* ---- xterritory_control_table ------------------------------------ */

os_error *xterritory_control_table(territory_t territory,
    char **table)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_plain_table -------------------------------------- */

os_error *xterritory_plain_table(territory_t territory,
    char **table)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_value_table -------------------------------------- */

os_error *xterritory_value_table(territory_t territory,
    char **table)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_representation_table ----------------------------- */

os_error *xterritory_representation_table(territory_t territory,
    char **table)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_collate ------------------------------------------ */

os_error *xterritory_collate(territory_t territory,
    char *s1,
    char *s2,
    territory_collate_flags flags,
    int *collate)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_read_boolean_symbols ----------------------------- */

os_error *xterritory_read_boolean_symbols(territory_t territory,
    territory_symbol_no symbol_no,
    osbool *symbol)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_read_integer_symbols ----------------------------- */

os_error *xterritory_read_integer_symbols(territory_t territory,
    territory_symbol_no symbol_no,
    int *symbol)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_read_string_symbols ------------------------------ */

os_error *xterritory_read_string_symbols(territory_t territory,
    territory_symbol_no symbol_no,
    char **symbol)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_read_calendar_information ------------------------ */

os_error *xterritory_read_calendar_information(territory_t territory,
    os_date_and_time *date_and_time,
    territory_calendar *calendar)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_name_to_number ----------------------------------- */

os_error *xterritory_name_to_number(territory_t territory,
    char *name,
    territory_t *territory_out)
{
    return ERR_NO_SUCH_SWI();
}

/* ---- xterritory_transform_string --------------------------------- */

os_error *xterritory_transform_string(territory_t territory,
    char *buffer,
    char *s,
    int size,
    int *used)
{
    return ERR_NO_SUCH_SWI();
}

