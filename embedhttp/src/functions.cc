/*  Copyright (C) 2004 Aleksey Krivoshey
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <embedhttp/main.h>

static struct HTTPContentType
content_map[] = {
        { "",           "text/plain",                   "",             "", },
        { ".html",      "text/html",                    "",             "", },
        { ".htm",       "text/html",                    "",             "", },
        { ".gif",       "image/gif",                    "",             "", },
        { ".jpeg",      "image/jpeg",                   "",             "", },
        { ".jpg",       "image/jpeg",                   "",             "", },
        { ".jpe",       "image/jpeg",                   "",             "", },
        { ".png",       "image/png",                    "",             "", },
        { ".mp3",       "audio/mpeg",                   "",             "", },
        { ".swf",       "application/x-shockwave-flash","",             "", },
        { ".dcr",       "application/x-director",       "",             "", },
        { ".pac",       "application/x-ns-proxy-autoconfig", "",        "", },
        { ".pa",        "application/x-ns-proxy-autoconfig", "",        "", },
        { ".tar",       "application/x-tar",              "",             "", },
        { ".gtar",      "application/x-gtar",             "",             "", },
        { ".tar.Z",     "application/x-tar",              "x-compress",   "compress", },
        { ".tar.gz",    "application/x-tar",              "x-gzip",       "gzip", },
        { ".taz",       "application/x-tar",              "x-gzip",       "gzip", },
        { ".tgz",       "application/x-tar",              "x-gzip",       "gzip", },
        { ".tar.z",     "application/x-tar",              "x-pack",       "x-pack", },
        { ".Z",         "application/x-compress",       "x-compress",   "compress", },
        { ".gz",        "application/x-gzip",           "x-gzip",       "gzip", },
        { ".z",         "unknown",                      "x-pack",       "x-pack", },
        { ".bz2",       "application/x-bzip2",          "x-bzip2",      "x-bzip2", },
        { ".ogg",       "application/x-ogg",            "",             "", },
        { ".hqx",       "application/mac-binhex40",     "",             "", },
        { ".cpt",       "application/mac-compactpro",   "",             "", },
        { ".doc",       "application/msword",           "",             "", },
        { ".bin",       "application/octet-stream",     "",             "", },
        { ".dms",       "application/octet-stream",     "",             "", },
        { ".lha",       "application/octet-stream",     "",             "", },
        { ".lzh",       "application/octet-stream",     "",             "", },
        { ".exe",       "application/octet-stream",     "",             "", },
        { ".class",     "application/octet-stream",     "",             "", },
        { ".oda",       "application/oda",              "",             "", },
        { ".pdf",       "application/pdf",              "",             "", },
        { ".ai",        "application/postscript",       "",             "", },
        { ".eps",       "application/postscript",       "",             "", },
        { ".ps",        "application/postscript",       "",             "", },
        { ".ppt",       "application/powerpoint",       "",             "", },
        { ".rtf",       "application/rtf",              "",             "", },
        { ".bcpio",     "application/x-bcpio",          "",             "", },
        { ".vcd",       "application/x-cdlink",         "",             "", },
        { ".cpio",      "application/x-cpio",           "",             "", },
        { ".csh",       "application/x-csh",            "",             "", },
        { ".dir",       "application/x-director",       "",             "", },
        { ".dxr",       "application/x-director",       "",             "", },
        { ".dvi",       "application/x-dvi",            "",             "", },
        { ".hdf",       "application/x-hdf",            "",             "", },
        { ".cgi",       "application/x-httpd-cgi",      "",             "", },
        { ".skp",       "application/x-koan",           "",             "", },
        { ".skd",       "application/x-koan",           "",             "", },
        { ".skt",       "application/x-koan",           "",             "", },
        { ".skm",       "application/x-koan",           "",             "", },
        { ".latex",     "application/x-latex",          "",             "", },
        { ".mif",       "application/x-mif",            "",             "", },
        { ".nc",        "application/x-netcdf",         "",             "", },
        { ".cdf",       "application/x-netcdf",         "",             "", },
        { ".patch",     "application/x-patch",          "",             "", },
        { ".sh",        "application/x-sh",             "",             "", },
        { ".shar",      "application/x-shar",           "",             "", },
        { ".sit",       "application/x-stuffit",        "",             "", },
        { ".sv4cpio",   "application/x-sv4cpio",        "",             "", },
        { ".sv4crc",    "application/x-sv4crc",         "",             "", },
        { ".tar",       "application/x-tar",            "",             "", },
        { ".tcl",       "application/x-tcl",            "",             "", },
        { ".tex",       "application/x-tex",            "",             "", },
        { ".texinfo",   "application/x-texinfo",        "",             "", },
        { ".texi",      "application/x-texinfo",        "",             "", },
        { ".t",         "application/x-troff",          "",             "", },
        { ".tr",        "application/x-troff",          "",             "", },
        { ".roff",      "application/x-troff",          "",             "", },
        { ".man",       "application/x-troff-man",      "",             "", },
        { ".me",        "application/x-troff-me",       "",             "", },
        { ".ms",        "application/x-troff-ms",       "",             "", },
        { ".ustar",     "application/x-ustar",          "",             "", },
        { ".src",       "application/x-wais-source",    "",             "", },
        { ".zip",       "application/zip",              "",             "", },
        { ".au",        "audio/basic",                  "",             "", },
        { ".snd",       "audio/basic",                  "",             "", },
        { ".mpga",      "audio/mpeg",                   "",             "", },
        { ".mp2",       "audio/mpeg",                   "",             "", },
        { ".aif",       "audio/x-aiff",                 "",             "", },
        { ".aiff",      "audio/x-aiff",                 "",             "", },
        { ".aifc",      "audio/x-aiff",                 "",             "", },
        { ".ram",       "audio/x-pn-realaudio",         "",             "", },
        { ".rpm",       "audio/x-pn-realaudio-plugin",  "",             "", },
        { ".ra",        "audio/x-realaudio",            "",             "", },
        { ".wav",       "audio/x-wav",                  "",             "", },
        { ".pdb",       "chemical/x-pdb",               "",             "", },
        { ".xyz",       "chemical/x-pdb",               "",             "", },
        { ".ief",       "image/ief",                    "",             "", },
        { ".tiff",      "image/tiff",                   "",             "", },
        { ".tif",       "image/tiff",                   "",             "", },
        { ".ras",       "image/x-cmu-raster",           "",             "", },
        { ".pnm",       "image/x-portable-anymap",      "",             "", },
        { ".pbm",       "image/x-portable-bitmap",      "",             "", },
        { ".pgm",       "image/x-portable-graymap",     "",             "", },
        { ".ppm",       "image/x-portable-pixmap",      "",             "", },
        { ".rgb",       "image/x-rgb",                  "",             "", },
        { ".xbm",       "image/x-xbitmap",              "",             "", },
        { ".xpm",       "image/x-xpixmap",              "",             "", },
        { ".xwd",       "image/x-xwindowdump",          "",             "", },
        { ".txt",       "text/plain",                   "",             "", },
        { ".rtx",       "text/richtext",                "",             "", },
        { ".tsv",       "text/tab-separated-values",    "",             "", },
        { ".etx",       "text/x-setext",                "",             "", },
        { ".sgml",      "text/x-sgml",                  "",             "", },
        { ".sgm",       "text/x-sgml",                  "",             "", },
        { ".mpeg",      "video/mpeg",                   "",             "", },
        { ".mpg",       "video/mpeg",                   "",             "", },
        { ".mpe",       "video/mpeg",                   "",             "", },
        { ".qt",        "video/quicktime",              "",             "", },
        { ".mov",       "video/quicktime",              "",             "", },
        { ".avi",       "video/x-msvideo",              "",             "", },
        { ".movie",     "video/x-sgi-movie",            "",             "", },
        { ".ice",       "x-conference/x-cooltalk",      "",             "", },
        { ".wrl",       "x-world/x-vrml",               "",             "", },
        { ".vrml",      "x-world/x-vrml",               "",             "", },
        { NULL,         NULL,                           NULL,           NULL, },
};


const struct HTTPContentType * get_content_type(const char * filename)
{
    size_t len = strlen(filename), nlen;
    struct HTTPContentType *content;

    for (content = &content_map[1]; content->name; content++) {
        nlen = strlen(content->name);
        if (nlen > len || strcasecmp(content->name, filename + (len - nlen)) != 0)
            continue;
        return content;
    }
    return &content_map[0];
}

static const unsigned char pr2six[256] =
{
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

int embedhttp_base64decode_len(const char *bufcoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (pr2six[*(bufin++)] <= 63);

    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    return nbytesdecoded + 1;
}

int embedhttp_base64decode(char *bufplain, const char *bufcoded)
{
    int len;

    len = embedhttp_base64decode_binary((unsigned char *) bufplain, bufcoded);
    bufplain[len] = '\0';
    return len;
}

/* This is the same as base64udecode() except on EBCDIC machines, where
 * the conversion of the output to ebcdic is left out.
 */
int embedhttp_base64decode_binary(unsigned char *bufplain, const char *bufcoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register unsigned char *bufout;
    register int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (pr2six[*(bufin++)] <= 63);
    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (unsigned char *) bufplain;
    bufin = (const unsigned char *) bufcoded;

    while (nprbytes > 4) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ingore that case */
    if (nprbytes > 1) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
    }
    if (nprbytes > 2) {
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    }
    if (nprbytes > 3) {
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    }

    nbytesdecoded -= (4 - nprbytes) & 3;
    return nbytesdecoded;
}

static const char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int embedhttp_base64encode_len(int len)
{
    return ((len + 2) / 3 * 4) + 1;
}

int embedhttp_base64encode(char *encoded, const char *string, int len)
{
    return embedhttp_base64encode_binary(encoded, (const unsigned char *) string, len);
}

/* This is the same as base64encode() except on EBCDIC machines, where
 * the conversion of the input to ascii is left out.
 */
int embedhttp_base64encode_binary(char *encoded, const unsigned char *string, int len)
{
    int i;
    char *p;

    p = encoded;
    for (i = 0; i < len - 2; i += 3) {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        *p++ = basis_64[((string[i] & 0x3) << 4) |
                        ((int) (string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                        ((int) (string[i + 2] & 0xC0) >> 6)];
        *p++ = basis_64[string[i + 2] & 0x3F];
    }
    if (i < len) {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        if (i == (len - 1)) {
            *p++ = basis_64[((string[i] & 0x3) << 4)];
            *p++ = '=';
        }
        else {
            *p++ = basis_64[((string[i] & 0x3) << 4) |
                            ((int) (string[i + 1] & 0xF0) >> 4)];
            *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }

    *p++ = '\0';
    return p - encoded;
}

unsigned char embedhttp_hex2char(unsigned char c)
{
    if (c < '@')
        return c - '0';
    return (c & 0x0f) + 9;
}

char * embedhttp_chop_string(const char * str)
{
    char * ret_str = (char*)str;
    char * rstr;
    if(ret_str != NULL){
        while(*ret_str && isspace(*ret_str)){
            ret_str++;
        }
        rstr = (char*)str+strlen(str)-1;
        while(rstr>ret_str && isspace(*rstr)){
            rstr--;
        }
        *(rstr+1) = 0;
    }
    return ret_str;
}

void embedhttp_makeTimeString(char * buf, time_t clock)
{
    struct  tm *ptime;

    if(clock == 0)
        clock = time(NULL);
    ptime = gmtime((time_t*)&clock);
    strftime(buf, 40, "%a, %d %b %Y %T GMT", ptime);
}

