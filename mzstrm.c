/* mzstrm.c -- Stream interface
   part of the MiniZip project

   Copyright (C) 2012-2017 Nathan Moinvaziri
     https://github.com/nmoinvaz/minizip
   Modifications for Zip64 support
     Copyright (C) 2009-2010 Mathias Svensson
     http://result42.com
   Copyright (C) 1998-2010 Gilles Vollant
     http://www.winimage.com/zLibDll/minizip.html

   This program is distributed under the terms of the same license as zlib.
   See the accompanying LICENSE file for the full text of the license.
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <time.h>

#include "mzstrm.h"

/***************************************************************************/

int32_t mz_stream_open(void *stream, const char *path, int mode)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->open == NULL)
        return MZ_STREAM_ERROR;
    return strm->open(strm, path, mode);
}

int32_t mz_stream_is_open(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->is_open == NULL)
        return MZ_STREAM_ERROR;
    return strm->is_open(strm);
}

int32_t mz_stream_read(void *stream, void* buf, uint32_t size)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->read == NULL)
        return MZ_STREAM_ERROR;
    if (strm->is_open != NULL && strm->is_open(strm) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->read(strm, buf, size);
}

int32_t mz_stream_read_uint8(void *stream, uint8_t *value)
{
    uint8_t c = 0;
    
    if (mz_stream_read(stream, &c, 1) == 1)
        *value = (uint8_t)c;
    else if (mz_stream_error(stream))
        return MZ_STREAM_ERROR;

    return MZ_OK;
}

int32_t mz_stream_read_uint16(void *stream, uint16_t *value)
{
    uint8_t c = 0;

    *value = 0;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value = (uint16_t)c;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint16_t)c) << 8;

    return MZ_OK;
}

int32_t mz_stream_read_uint32(void *stream, uint32_t *value)
{
    uint8_t c = 0;

    *value = 0;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value = (uint32_t)c;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint32_t)c) << 8;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint32_t)c) << 16;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint32_t)c) << 24;

    return MZ_OK;
}

int32_t mz_stream_read_uint64(void *stream, uint64_t *value)
{
    uint8_t c = 0;

    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value = (uint64_t)c;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint64_t)c) << 8;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint64_t)c) << 16;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint64_t)c) << 24;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint64_t)c) << 32;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint64_t)c) << 40;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint64_t)c) << 48;
    if (mz_stream_read_uint8(stream, &c) != MZ_OK)
        return MZ_STREAM_ERROR;
    *value += ((uint64_t)c) << 56;

    return MZ_OK;
}

int32_t mz_stream_write(void *stream, const void *buf, uint32_t size)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->write == NULL)
        return MZ_STREAM_ERROR;
    if (size == 0)
        return size;
    if (strm->is_open != NULL && strm->is_open(strm) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->write(strm, buf, size);
}

static int32_t mz_stream_write_value(void *stream, uint64_t value, uint32_t len)
{
    uint8_t buf[8];
    uint32_t n = 0;

    for (n = 0; n < len; n++)
    {
        buf[n] = (uint8_t)(value & 0xff);
        value >>= 8;
    }

    if (value != 0)
    {
        // Data overflow - hack for ZIP64 (X Roche)
        for (n = 0; n < len; n++)
            buf[n] = 0xff;
    }

    if (mz_stream_write(stream, buf, len) != len)
        return MZ_STREAM_ERROR;

    return MZ_OK;
}

int32_t mz_stream_write_uint8(void *stream, uint8_t value)
{
    return mz_stream_write_value(stream, value, sizeof(uint8_t));
}

int32_t mz_stream_write_uint16(void *stream, uint16_t value)
{
    return mz_stream_write_value(stream, value, sizeof(uint16_t));
}

int32_t mz_stream_write_uint32(void *stream, uint32_t value)
{
    return mz_stream_write_value(stream, value, sizeof(uint32_t));
}

int32_t mz_stream_write_uint64(void *stream, uint64_t value)
{
    return mz_stream_write_value(stream, value, sizeof(uint64_t));
}

int32_t mz_stream_copy(void *target, void *source, int32_t len)
{
    uint8_t buf[UINT16_MAX];
    int32_t bytes_to_copy = 0;
    int32_t read = 0;
    int32_t written = 0;

    while (len > 0)
    {
        bytes_to_copy = len;
        if (bytes_to_copy > UINT16_MAX)
            bytes_to_copy = UINT16_MAX;
        read = mz_stream_read(source, buf, bytes_to_copy);
        if (read < 0)
            return MZ_STREAM_ERROR;
        written = mz_stream_write(target, buf, read);
        if (written != read)
            return MZ_STREAM_ERROR;
        len -= read;
    }

    return MZ_OK;
}

int64_t mz_stream_tell(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->tell == NULL)
        return MZ_STREAM_ERROR;
    if (strm->is_open != NULL && strm->is_open(strm) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->tell(strm);
}

int32_t mz_stream_seek(void *stream, uint64_t offset, int origin)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->seek == NULL)
        return MZ_STREAM_ERROR;
    if (strm->is_open != NULL && strm->is_open(strm) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->seek(strm, offset, origin);
}

int32_t mz_stream_close(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->close == NULL)
        return MZ_STREAM_ERROR;
    return strm->close(strm);
}

int32_t mz_stream_error(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->error == NULL)
        return MZ_STREAM_ERROR;
    return strm->error(strm);
}

int32_t mz_stream_set_base(void *stream, void *base)
{
    mz_stream *strm = (mz_stream *)stream;
    strm->base = (mz_stream *)base;
    return MZ_OK;
}

int64_t mz_stream_get_total_in(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm->get_total_in == NULL)
        return MZ_STREAM_ERROR;
    return strm->get_total_in(stream);
}

int64_t mz_stream_get_total_out(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm->get_total_out == NULL)
        return MZ_STREAM_ERROR;
    return strm->get_total_out(stream);
}

void *mz_stream_create(void **stream)
{
    mz_stream *strm = NULL;
    if (stream == NULL)
        return NULL;
    strm = (mz_stream *)*stream;
    return strm->create(stream);
}

void mz_stream_delete(void **stream)
{
    mz_stream *strm = NULL;
    if (stream == NULL)
        return;
    strm = (mz_stream *)*stream;
    strm->delete(stream);
}

/***************************************************************************/

typedef struct mz_stream_passthru_s {
    mz_stream   stream;
    int64_t     total_in;
    int64_t     total_out;
} mz_stream_passthru;

/***************************************************************************/

int32_t mz_stream_passthru_open(void *stream, const char *path, int mode)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    return mz_stream_open(passthru->stream.base, path, mode);
}

int32_t mz_stream_passthru_is_open(void *stream)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    return mz_stream_is_open(passthru->stream.base);
}

int32_t mz_stream_passthru_read(void *stream, void *buf, uint32_t size)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    int32_t read = mz_stream_read(passthru->stream.base, buf, size);
    if (read > 0)
        passthru->total_in += read;
    return read;
}

int32_t mz_stream_passthru_write(void *stream, const void *buf, uint32_t size)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    int32_t written = mz_stream_write(passthru->stream.base, buf, size);
    if (written > 0)
        passthru->total_out += written;
    return written;
}

int64_t mz_stream_passthru_tell(void *stream)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    return mz_stream_tell(passthru->stream.base);
}

int32_t mz_stream_passthru_seek(void *stream, uint64_t offset, int origin)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    return mz_stream_seek(passthru->stream.base, offset, origin);
}

int32_t mz_stream_passthru_close(void *stream)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    return mz_stream_close(passthru->stream.base);
}

int32_t mz_stream_passthru_error(void *stream)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    return mz_stream_error(passthru->stream.base);
}

int64_t mz_stream_passthru_get_total_in(void *stream)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    return passthru->total_in;
}

int64_t mz_stream_passthru_get_total_out(void *stream)
{
    mz_stream_passthru *passthru = (mz_stream_passthru *)stream;
    return passthru->total_out;
}

void *mz_stream_passthru_create(void **stream)
{
    mz_stream_passthru *passthru = NULL;

    passthru = (mz_stream_passthru *)malloc(sizeof(mz_stream_passthru));
    if (passthru != NULL)
    {
        memset(passthru, 0, sizeof(mz_stream_passthru));

        passthru->stream.open = mz_stream_passthru_open;
        passthru->stream.is_open = mz_stream_passthru_is_open;
        passthru->stream.read = mz_stream_passthru_read;
        passthru->stream.write = mz_stream_passthru_write;
        passthru->stream.tell = mz_stream_passthru_tell;
        passthru->stream.seek = mz_stream_passthru_seek;
        passthru->stream.close = mz_stream_passthru_close;
        passthru->stream.error = mz_stream_passthru_error;
        passthru->stream.create = mz_stream_passthru_create;
        passthru->stream.delete = mz_stream_passthru_delete;
        passthru->stream.get_total_in = mz_stream_passthru_get_total_in;
        passthru->stream.get_total_out = mz_stream_passthru_get_total_out;
    }
    if (stream != NULL)
        *stream = passthru;

    return passthru;
}

void mz_stream_passthru_delete(void **stream)
{
    mz_stream_passthru *passthru = NULL;
    if (stream == NULL)
        return;
    passthru = (mz_stream_passthru *)*stream;
    if (passthru != NULL)
        free(passthru);
}

/***************************************************************************/

int32_t mz_os_file_exists(const char *path)
{
    void *stream = NULL;
    int opened = 0;

    mz_stream_os_create(&stream);

    if (mz_stream_os_open(stream, path, MZ_STREAM_MODE_READ) == MZ_OK)
    {
        mz_stream_os_close(stream);
        opened = 1;
    }

    mz_stream_os_delete(&stream);

    return opened;
}

int64_t mz_os_file_get_size(const char *path)
{
    void *stream = NULL;
    int64_t size = 0;
    
    mz_stream_os_create(&stream);

    if (mz_stream_os_open(stream, path, MZ_STREAM_MODE_READ) == MZ_OK)
    {
        mz_stream_os_seek(stream, 0, MZ_STREAM_SEEK_END);
        size = mz_stream_os_tell(stream);
        mz_stream_os_close(stream);
    }

    mz_stream_os_delete(&stream);

    return size;
}

/***************************************************************************/

int mz_invalid_date(const struct tm *ptm)
{
#define datevalue_in_range(min, max, value) ((min) <= (value) && (value) <= (max))
    return (!datevalue_in_range(0, 207, ptm->tm_year) ||
            !datevalue_in_range(0, 11, ptm->tm_mon) ||
            !datevalue_in_range(1, 31, ptm->tm_mday) ||
            !datevalue_in_range(0, 23, ptm->tm_hour) ||
            !datevalue_in_range(0, 59, ptm->tm_min) ||
            !datevalue_in_range(0, 59, ptm->tm_sec));
#undef datevalue_in_range
}

// Conversion without validation
void mz_dosdate_to_raw_tm(uint64_t dos_date, struct tm *ptm)
{
    uint64_t date = (uint64_t)(dos_date >> 16);

    ptm->tm_mday = (uint16_t)(date & 0x1f);
    ptm->tm_mon = (uint16_t)(((date & 0x1E0) / 0x20) - 1);
    ptm->tm_year = (uint16_t)(((date & 0x0FE00) / 0x0200) + 80);
    ptm->tm_hour = (uint16_t)((dos_date & 0xF800) / 0x800);
    ptm->tm_min = (uint16_t)((dos_date & 0x7E0) / 0x20);
    ptm->tm_sec = (uint16_t)(2 * (dos_date & 0x1f));
    ptm->tm_isdst = -1;
}

int32_t mz_dosdate_to_tm(uint64_t dos_date, struct tm *ptm)
{
    mz_dosdate_to_raw_tm(dos_date, ptm);

    if (mz_invalid_date(ptm))
    {
        // Invalid date stored, so don't return it.
        memset(ptm, 0, sizeof(struct tm));
        return -1;
    }
    return 0;
}

time_t mz_dosdate_to_time_t(uint64_t dos_date)
{
    struct tm ptm;
    mz_dosdate_to_raw_tm(dos_date, &ptm);
    return mktime(&ptm);
}

uint32_t mz_tm_to_dosdate(const struct tm *ptm)
{
    struct tm fixed_tm = { 0 };

    // Years supported:
    // [00, 79]      (assumed to be between 2000 and 2079)
    // [80, 207]     (assumed to be between 1980 and 2107, typical output of old
    //                software that does 'year-1900' to get a double digit year)
    // [1980, 2107]  (due to the date format limitations, only years between 1980 and 2107 can be stored.)

    memcpy(&fixed_tm, ptm, sizeof(struct tm));
    if (fixed_tm.tm_year >= 1980) // range [1980, 2107]
        fixed_tm.tm_year -= 1980;
    else if (fixed_tm.tm_year >= 80) // range [80, 99] 
        fixed_tm.tm_year -= 80;
    else // range [00, 79]
        fixed_tm.tm_year += 20;

    if (mz_invalid_date(ptm))
        return 0;

    return (uint32_t)(((fixed_tm.tm_mday) + (32 * (fixed_tm.tm_mon + 1)) + (512 * fixed_tm.tm_year)) << 16) |
        ((fixed_tm.tm_sec / 2) + (32 * fixed_tm.tm_min) + (2048 * (uint32_t)fixed_tm.tm_hour));
}
