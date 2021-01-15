/*

@file gidunit.h - Header file for a C testing framework that is designed to
      make writing thorough tests for C code as simple as possible.

@author - Aaron Leggett
@repo - https://github.com/a-leggett/GIDUnit

@license - This software is available under two licenses:
   License 1 - The Unlicense (fulltext below)
   License 2 - The MIT license (fulltext below)
   You may pick whichever of these two licenses you prefer.

   ============================ License 1 ============================
   The Unlicense

   This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <https://unlicense.org>


   ============================ License 2 ============================
   MIT License

   Copyright (c) 2021 Aaron Leggett

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

@example

#include "gidunit.h"

BEGIN_TEST_SUITE(MyFirstTestSuite)

  SetUp()
  {
    fixture = malloc(sizeof(int));
    assert_not_null(fixture);
    *(int*)fixture = 1;
  }

  TearDown()
  {
    free(fixture);
  }

  Test(MyFirstTest)
  {
    assert_not_null(fixture);
    assert_int_eq(1, *(int*)fixture);
  }

  Test(MyParameterizedTest,
    IntRow(1, 2, 4, 8, 16)
    IntRow(0, 0, 0, 0, 0)
    IntRow(1, 1, 1, 1, 2)
    EnumParam(i, 3, 6, 9, 12)
    StringEnumParam(word, "Alpha", "Bravo", "Charlie", "Delta", "Echo"))
  {
    //This test will run 3*4*5=60 times
    assert_int_eq(1, *(int*)fixture);
    assert_int_not_eq(0, int_row[0]);
    assert_string_not_eq("Alpha", word);
    assert_int_not_eq(12, i);
  }

END_TEST_SUITE()


BEGIN_TEST_SUITE(MySecondTestSuite)

  Test(MyOtherTest,
    StringRow("Alpha", "Bravo", "Charlie")
    StringRow("alpha", "bravo", "charlie")
    StringRow("ALPHA", "BRAVO", "CHARLIE"))
  {
    assert_null(fixture);
    assert_int_eq(1, 1);
    assert_string_not_eq("alpha", string_row[0]);
  }

END_TEST_SUITE()

int main()
{
  ADD_TEST_SUITE(MyFirstTestSuite);
  ADD_TEST_SUITE(MySecondTestSuite);
  return gidunit();
}

*/



#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

#ifndef GIDUNIT_H
#define GIDUNIT_H

/* The maximum length of an assert failure message. */
#define GID_MAX_MESSAGE_LENGTH (256)

/*  The maximum length of the string representation of a test configuration
 * (that is, the string that shows the values of each variable in a test). */
#define GID_MAX_CONFIGURATION_STRING_LENGTH (256)

/* The amount of corruption-detection padding to add to the ends of memory
 * that is allocated by the gid_malloc function. */
#define GID_MALLOC_PADDING (32)

/* Checks how many bytes of memory are equivalent.
 * @param a - Pointer to the first memory object.
 * @param b - Pointer to the second memory object.
 * @param size - The number of bytes to scan.
 * @returns - The number of bytes that were equal in both 'a' and 'b'. If the
 * returned value is equal to 'size', then both 'a' and 'b' are equal. */
size_t _gid_cmp_memory(const uint8_t* a, const uint8_t* b, size_t size)
{
  size_t match = 0;
  for(size_t i = 0; i < size; i++)
  {
    if(a[i] == b[i])
      match++;
    else
      break;
  }
  return match;
}

/* Clones a string.
 * @param src - The null-terminated source string.
 * @returns - A newly allocated clone of the 'src' string. */
const char* _gid_strclone(const char* src)
{
  if(src == NULL)
    return NULL;
  size_t len = strlen(src) + 1;//+1 for null terminator
  const char* ret = malloc(sizeof(char) * len);
  memcpy((char*)ret, (char*)src, len);
  return ret;
}

/* Base structure for a test parameter. Each test can have multiple parameters,
 * and each test will run with all combinations of parameter values. Parameter
 * values are abstractly defined in this structure, and concretely defined in
 * various structures that will be stored in the 'data' field. */
typedef struct GIDParamBase
{
  /* Stores the implementation-specific data for the parameter, such as the set
   * of possible values and the current value. */
  void* data;

  /* The name of the parameter. */
  const char* name;

  /* Pointer to a function that counts the number of stored values. The 'data'
   * field in this GIDParamBase structure will be passed as the only
   * argument. The function returns the number of values. */
  size_t (*value_count)(const void* data);

  /* Pointer to a function that gets a pointer to the current value.
   * The 'data' field of this GIDParamBase structure will be passed as the
   * only argument. The function returns a pointer to the current value. */
  void* (*current_value)(const void* data);

  /* Pointer to a function that generates a string representation of the
   * current value. The 'data' field in this GIDParamBase structure will be
   * passed as the first argument. The second argument is the destination
   * buffer into which the string will be written (it may be NULL for
   * counting the length of the string), and the third argument is the size
   * of the destination buffer. The function returns the size of the string
   * regardless of how many characters could fit in the destination buffer. */
  size_t (*current_value_string)(const void* data, char* dst, size_t dstSize);

  /* Pointer to a function that changes the value of this parameter to the 'next
   * value', if possible. The 'data' field in this GIDParamBase structure will
   * be the only argment, and the function returns non-zero if there was a 'next
   * value', otherwise zero. */
  int (*next_value)(void* data);

  /* Pointer to a function that resets the value of this parameter to the
   * initial value. The 'data' field in this GIDParamBase structure will be
   * the only argument, and the function does not have a return value. */
  void (*reset_value)(void* data);

  /* Pointer to a function that frees any memory that was allocated by the
   * implementation. The 'data' parameter in this GIDParamBase structure will
   * be the only argument, and the implementation is required to free it. */
  void (*free_data)(void* data);

  /* Pointer to the next linked parameter, or NULL. */
  struct GIDParamBase* next;
} GIDParamBase;

/* Gets the number of values in a parameter.
 * @param param - Pointer to the GIDParamBase from which to get the number of
 *        values.
 * @returns - The number of values. May be zero, in which case the parameter
 *          should be considered unused (this is the case with 'row'
 *          parameters). */
size_t _gid_param_value_count(const GIDParamBase* param)
{
  return param->value_count(param->data);
}

/* Gets the current value of a parameter.
 * @param param - Pointer to the GIDParamBase from which to get the value.
 * @returns - Pointer to the value. */
void* _gid_param_get_value(const GIDParamBase* param)
{
  return param->current_value(param->data);
}

/* Generates a string representation of the current value of a parameter.
 * @param param - Pointer to the GIDParamBase from which to get the value
 *        string.
 * @param dst - Destination buffer, may be NULL to count the length of the
 *        string.
 * @param dstSize - The size of the destination buffer.
 * @returns - The size of the value string, regardless of how many bytes
 *            could fit in the dst buffer, excluding the null terminator. */
size_t _gid_param_get_value_string(const GIDParamBase* param, char* dst, size_t dstSize)
{
  return param->current_value_string(param->data, dst, dstSize);
}

/* Changes the value of a parameter to the 'next value', if any.
 * @param param - Pointer to the GIDParamBase on which to change
 *        the value.
 * @returns - Non-zero if the value was changed. Zero means that
 *            there are no more values. */
int _gid_param_next_value(GIDParamBase* param)
{
  return param->next_value(param->data);
}

/* Resets a parameter to its initial value.
 * @param param - Pointer to the GIDParamBase on which to reset the value. */
void _gid_param_reset_value(GIDParamBase* param)
{
  param->reset_value(param->data);
}

/* Gets a string representation of a linked list of parameters.
 * @param root - The root GIDParamBase to stringify. All GIDParamBases
 *        that are linked to this root will be stringified.
 * @param dst - The destination buffer.
 * @param dstSize - The size of the destination buffer.
 * @returns - The total size of the required string, regardless of how
 * many bytes were actually written to the destination buffer. */
int64_t _gid_get_params_string(GIDParamBase* root, char* dst, int64_t dstSize)
{
  int64_t pos = 0;
  int first = 1;
  while(root != NULL)
  {
    if(_gid_param_value_count(root) == 0)
    {
      root = root->next;
      continue;//Don't print params with no values
    }
    int64_t valueLen = _gid_param_get_value_string(root, NULL, 0);

    if(!first)
    {
      if(dst != NULL && pos + 2 < dstSize)
      {
        dst[pos + 0] = ',';
        dst[pos + 1] = ' ';
      }
      pos += 2;
    }
    else
    {
      first = 0;
    }

    if(dst != NULL && dstSize > pos)
      strncpy(dst+pos, root->name, dstSize-pos);
    pos += strlen(root->name);

    if(dst != NULL && pos + 1 < dstSize)
      dst[pos] = '=';
    pos++;

    if(dst != NULL && dstSize > pos)
      _gid_param_get_value_string(root, dst+pos, dstSize-pos);
    pos += valueLen;

    if(root->next != NULL)
      root = root->next;
    else
      break;
  }

  if(pos + 1 < dstSize)
    dst[pos] = '\0';
  else if(dstSize > 0)
    dst[dstSize - 1] = '\0';

  return pos;
}

/* Frees memory that was allocated for a parameter.
 * @param param - Pointer to the parameter's GIDParamBase. */
void _gid_param_free(GIDParamBase* param)
{
  param->free_data(param->data);
  free((char*)param->name);
  free(param);
}

/* Frees all memory in a linked list of parameters.
 * @param root - Pointer to the first GIDParamBase in the linked list of
 *        parameters to free. */
void _gid_free_params(GIDParamBase* root)
{
  GIDParamBase* cur = root;
  while(cur != NULL)
  {
    GIDParamBase* next = cur->next;
    _gid_param_free(cur);
    cur = next;
  }
}

/* Defines the type of data stored in a GIDRowParam. */
typedef enum GIDRowParamType
{
  /* A 64-bit signed integer is stored. */
  GID_ROW_PARAM_TYPE_INT64,

  /* A 64-bit unsigned integer is stored. */
  GID_ROW_PARAM_TYPE_UINT64,

  /* A pointer to a null-terminated string is stored. */
  GID_ROW_PARAM_TYPE_STRING,
} GIDRowParamType;

/* Contains the value of one row in a GIDRowParam.
 * A single value consists of multiple columns. */
typedef struct GIDRowParamValue
{
  /* Pointer to the memory of the values in the columns. */
  void* cols;

  /* The number of stored columns. */
  size_t col_count;

  /* Pointer to the next linked GIDRowParamValue, or NULL if none. */
  struct GIDRowParamValue* next_row;
} GIDRowParamValue;

/* Creates a GIDRowParamValue in memory.
 * @param cols - Pointer to the values to store in the columns. This data
 *        will be copied.
 * @param valueSize - The size, in bytes, of each column value.
 * @param colCount - The number of columns.
 * @param type - The GIDRowParamType that defines the type of data that is
 *        stored in each column.
 * @returns - A pointer to the allocated GIDRowParamValue. */
GIDRowParamValue* _gid_create_row_param_value(
  const void* cols,
  size_t valueSize,
  size_t colCount,
  GIDRowParamType type)
{
  GIDRowParamValue* ret = malloc(sizeof(GIDRowParamValue));
  /* Add padding just in case a test accidentally goes out of bounds */
  size_t paddingColCount = 32;
  ret->cols = malloc((colCount + paddingColCount) * valueSize);

  if(type != GID_ROW_PARAM_TYPE_STRING)
  {
    memcpy((void*)ret->cols, cols, colCount * valueSize);
  }
  else
  {
    /*Copy the string values, not just the pointers*/
    for(size_t i = 0; i < colCount; i++)
      ((char**)ret->cols)[i] = (char*)_gid_strclone(((const char**)cols)[i]);
  }

  memset((uint8_t*)ret->cols+(colCount*valueSize), 0, paddingColCount * valueSize);
  ret->col_count = colCount;
  ret->next_row = NULL;
  return ret;
}

/* Contains data for a 'row parameter', which is a parameter that stores
 * multiple rows (defined by the test) each with multiple columns of values. */
typedef struct GIDRowParamData
{
  /* Pointer to the first GIDRowParamValue that is defined for the test that
   * contains this row parameter, or NULL if none. */
  GIDRowParamValue* first_row;

  /* Pointer to the last GIDRowParamValue that is defined for the test that
   * contains this row parameter, or NULL if none. */
  GIDRowParamValue* last_row;

  /* Pointer to the GIDRowParamValue that contains the columns that are
   * currently selected for this row parameter, or NULL if none. */
  GIDRowParamValue* current_row;

  /* The GIDRowParamType that defines the type of data stored in each column. */
  GIDRowParamType type;
} GIDRowParamData;

/* Gets the number of values in a 'row parameter'.
 * @param data - Pointer to the GIDRowParamData that was allocated for the
 *        row parameter.
 * @returns - The number of rows (each value is one row) in the parameter.  */
size_t _gid_row_param_value_count(const void* data)
{
  const GIDRowParamData* rowData = data;
  size_t count = 0;
  GIDRowParamValue* value = rowData->first_row;
  while(value != NULL)
  {
    count++;
    value = value->next_row;
  }
  return count;
}

/* Gets the current row value of a 'row parameter'.
 * @param data - Pointer to the GIDRowParamData.
 * @returns - Pointer to the GIDRowParamValue, or NULL if the row parameter
 *          is empty. */
void* _gid_row_param_get_current_value(const void* data)
{
  const GIDRowParamData* rowData = data;
  return rowData->current_row;
}

/* Gets a string representation of a 'row parameter'.
 * @param data - Pointer to the GIDRowParamData.
 * @param dst - The destination buffer.
 * @param dstSize - The size of the destination buffer.
 * @returns - The actual size of the string, regardless of how much could
 *          fit in the destination buffer. */
size_t _gid_row_param_get_current_value_string(
  const void* data,
  char* dst,
  size_t dstSize)
{
  const GIDRowParamData* rowData = data;

  size_t pos = 0;
  if(pos + 1 < dstSize)
    dst[pos] = '{';
  pos++;

  GIDRowParamValue* row = rowData->current_row;
  if(row != NULL)
  {
    for(size_t i = 0; i < row->col_count; i++)
    {
      if(rowData->type == GID_ROW_PARAM_TYPE_STRING)
      {
        if(pos + 1 < dstSize)
          dst[pos] = '\"';
        pos++;
      }

      switch(rowData->type)
      {
        case GID_ROW_PARAM_TYPE_INT64:
          pos += snprintf(dst != NULL ? dst+pos : NULL, dstSize > pos ? dstSize - pos : 0, "%"PRId64, ((int64_t*)row->cols)[i]);
          break;
        case GID_ROW_PARAM_TYPE_UINT64:
          pos += snprintf(dst != NULL ? dst+pos : NULL, dstSize > pos ? dstSize - pos : 0, "%"PRIu64, ((uint64_t*)row->cols)[i]);
          break;
        case GID_ROW_PARAM_TYPE_STRING:
          pos += snprintf(dst != NULL ? dst+pos : NULL, dstSize > pos ? dstSize - pos : 0, "%s", ((char**)row->cols)[i]);
          break;
        default:
          if(pos + 1 < dstSize && dst != NULL)
            dst[pos] = '?';
          pos++;
          break;
      }

      if(rowData->type == GID_ROW_PARAM_TYPE_STRING)
      {
        if(pos + 1 < dstSize)
          dst[pos] = '\"';
        pos++;
      }

      if(i + 1< row->col_count)
      {
        if(pos + 2 < dstSize)
        {
          dst[pos + 0] = ',';
          dst[pos + 1] = ' ';
        }
        pos += 2;
      }
    }
  }

  if(pos + 1 < dstSize)
    dst[pos] = '}';
  pos++;

  if(dstSize > 0)
  {
    size_t nullPos = pos;
    if(nullPos >= dstSize)
      nullPos = dstSize - 1;
    dst[nullPos] = '\0';
  }

  return pos;
}

/* Moves the 'current value' of a 'row parameter' to the next row, if any.
 * @param data - Pointer to the GIDRowParamData.
 * @returns - True if there was a 'next row', otherwise false. */
int _gid_row_param_next_value(void* data)
{
  GIDRowParamData* rowData = data;
  if(rowData->current_row != NULL)
  {
    if(rowData->current_row->next_row != NULL)
    {
      rowData->current_row = rowData->current_row->next_row;
      return 1;
    }
    else
    {
      //At last row
      return 0;
    }
  }
  else
  {
    //No values
    return 0;
  }
}

/* Resets the 'current value' of a 'row parameter' to the first
 * stored row, if any.
 * @param data - Pointer to the GIDRowParamData. */
void _gid_row_param_reset_value(void* data)
{
  GIDRowParamData* rowData = data;
  rowData->current_row = rowData->first_row;
}

/* Frees memory allocated for a row parameter.
 * @param data - Pointer to the GIDRowParamData to free. */
void _gid_row_param_free_data(void* data)
{
  GIDRowParamData* rowData = data;
  GIDRowParamValue* value = rowData->first_row;
  while(value != NULL)
  {
    GIDRowParamValue* next = value->next_row;
    if(rowData->type == GID_ROW_PARAM_TYPE_STRING)
    {
      //Free all strings from memory
      for(size_t i = 0; i < value->col_count; i++)
        free(((char**)value->cols)[i]);
    }

    free((void*)value->cols);
    free(value);
    value = next;
  }
  free(data);
}

/* Creates a row parameter.
 * @param name - The name of the parameter.
 * @param type - The GIDRowParamType that defines the type of data stored in
 *        each column.
 * @returns - Pointer to the GIDParamBase for the newly allocated row
 *          parameter. */
GIDParamBase* _gid_create_row_param(const char* name, GIDRowParamType type)
{
  GIDRowParamData* data = malloc(sizeof(GIDRowParamData));
  data->first_row = NULL;
  data->last_row = NULL;
  data->current_row = NULL;
  data->type = type;

  GIDParamBase* base = malloc(sizeof(GIDParamBase));
  base->data = data;
  base->name = _gid_strclone(name);
  base->value_count = _gid_row_param_value_count;
  base->current_value = _gid_row_param_get_current_value;
  base->current_value_string = _gid_row_param_get_current_value_string;
  base->next_value = _gid_row_param_next_value;
  base->reset_value = _gid_row_param_reset_value;
  base->free_data = _gid_row_param_free_data;
  base->next = NULL;
  return base;
}

/* Adds a row to a row parameter.
 * @param param - Pointer to the GIDParamBase of the destination row
 *        parameter.
 * @param value - Pointer to the GIDRowParamValue to add. */
void _gid_add_row_param_value(GIDParamBase* param, GIDRowParamValue* value)
{
  GIDRowParamData* data = param->data;
  GIDRowParamValue* prev = data->last_row;
  data->last_row = value;
  if(data->first_row == NULL)
  {
    data->first_row = value;
    data->current_row = value;
  }
  if(prev != NULL)
    prev->next_row = value;
}

/* Contains data for a 'range parameter', which is a parameter that contains
 * integer values ranging from a test-defined minimum and maximum value.
 * Note that the values are stored in signed 64-bit integers, but the parameter
 * has the option to treat them as unsigned. */
typedef struct GIDRangeParamData
{
  /* The minimum value. */
  const int64_t min;

  /* The maximum value. */
  const int64_t max;

  /* The current value. */
  int64_t current;

  /* Does the test want to treat the values in this parameter as though they
   * are signed? Non-zero means yes, the values are interpreted as signed. Zero
   * means that the values are interpreted as unsigned integers. */
  int is_signed;
} GIDRangeParamData;

/* Gets the number of values in a range parameter.
 * @param data - Pointer to the GIDRangeParamData.
 * @returns - The number of values. */
size_t _gid_range_param_value_count(const void* data)
{
  const GIDRangeParamData* rangeData = data;
  if(rangeData->is_signed)
    return (rangeData->max - rangeData->min) + 1;
  else
    return ((uint64_t)rangeData->max - (uint64_t)rangeData->min) + 1;
}

/* Gets the current value in a range parameter.
 * @param data - Pointer to the GIDRangeParamData.
 * @returns - Pointer to the value. */
void* _gid_range_param_get_current_value(const void* data)
{
  const GIDRangeParamData* rangeData = data;
  return (void*)&rangeData->current;
}

/* Gets a string representation of the current value in a range parameter.
 * @param data - Pointer to the GIDRangeParamData.
 * @param dst - The destination buffer.
 * @param dstSize - The size of the destination buffer.
 * @returns - The size of the string representation, regardless of how many
 *          characters were written to the destination buffer.*/
size_t _gid_range_param_get_current_value_string(
  const void* data,
  char* dst,
  size_t dstSize)
{
  const GIDRangeParamData* rangeData = data;
  const char* format = rangeData->is_signed ? "%"PRId64 : ""PRIu64;
  return snprintf(dst, dstSize, format, rangeData->current);
}

/* Increments the 'current value' of a range parameter, if it is below
 * the maximum value.
 * @param data - Pointer to the GIDRangeParamData.
 * @returns - Non-zero if the value was incremented. Zero means that the
 *            maximum value is already selected, thus it cannot increment. */
int _gid_range_param_next_value(void* data)
{
  GIDRangeParamData* rangeData = data;
  if((rangeData->is_signed && rangeData->current < rangeData->max) || (!rangeData->is_signed && (uint64_t)rangeData->current < (uint64_t)rangeData->max))
  {
    rangeData->current++;
    return 1;
  }
  else
  {
    return 0;
  }
}

/* Resets the 'current value' of a range parameter to its minimum.
 * @param data - Pointer to the GIDRangeParamData. */
void _gid_range_param_reset_value(void* data)
{
  GIDRangeParamData* rangeData = data;
  rangeData->current = rangeData->min;
}

/* Frees a GIDRangeParamData and all of its associated memory.
 * @param data - Pointer to the GIDRangeParamData. */
void _gid_range_param_free_data(void* data)
{
  free(data);
}

/* Creates a range parameter.
 * @param name - The name of the variable.
 * @param min - The minimum value of the parameter.
 * @param max - The maximum value of the parameter.
 * @param isSigned - Should the parameter's value be signed? If false (0), then
 *        the 'min' and 'max' arguments will be reinterpreted as unsigned.
 * @returns - Pointer to the GIDParamBase of the range parameter. */
GIDParamBase* _gid_create_range_param(const char* name, int64_t min, int64_t max, int isSigned)
{
  GIDRangeParamData* data = malloc(sizeof(GIDRangeParamData));
  memcpy((int64_t*)&data->min, &min, sizeof(data->min));
  memcpy((int64_t*)&data->current, &min, sizeof(data->current));
  memcpy((int64_t*)&data->max, &max, sizeof(data->max));
  data->is_signed = isSigned;

  GIDParamBase* base = malloc(sizeof(GIDParamBase));
  base->data = data;
  base->name = _gid_strclone(name);
  base->value_count = _gid_range_param_value_count;
  base->current_value = _gid_range_param_get_current_value;
  base->current_value_string = _gid_range_param_get_current_value_string;
  base->next_value = _gid_range_param_next_value;
  base->reset_value = _gid_range_param_reset_value;
  base->free_data = _gid_range_param_free_data;
  base->next = NULL;
  return base;
}

/* Defines the type of values stored in an 'enum parameter.' */
typedef enum GIDEnumParamType
{
  /* Signed 64-bit integer values. */
  GID_ENUM_PARAM_TYPE_INT64,

  /* Unsigned 64-bit integer values. */
  GID_ENUM_PARAM_TYPE_UINT64,

  /* Pointers to null-terminated strings. */
  GID_ENUM_PARAM_TYPE_STRING,
} GIDEnumParamType;

/* Contains the data for an 'enum parameter', which is a parameter that has
 * one value at a time from a set of values defined by the test. */
typedef struct GIDEnumParamData
{
  /* Pointer to the values defined by the test. */
  void* values;

  /* Defines the type of value stored. */
  GIDEnumParamType type;

  /* The size of each value. */
  size_t value_size;

  /* The number of values defined by the test. */
  size_t count;

  /* The zero-based index of the current value. */
  size_t index;
} GIDEnumParamData;

/* Gets the number of values in an 'enum parameter'.
 * @param data - Pointer to the GIDEnumParamData.
 * @returns - The number of values. */
size_t _gid_enum_param_value_count(const void* data)
{
  const GIDEnumParamData* paramData = data;
  return paramData->count;
}

/* Gets the current value of an 'enum parameter'.
 * @param data - Pointer to the GIDEnumParamData.
 * @returns - Pointer to the current value. */
void* _gid_enum_param_get_current_value(const void* data)
{
  const GIDEnumParamData* paramData = data;
  return ((uint8_t*)paramData->values)
    + (paramData->value_size * paramData->index);
}

/* Gets a string representation of the current value of an 'enum parameter.'
 * @param data - Pointer to the GIDEnumParamData.
 * @param dst - Destination buffer.
 * @param dstSize - The size of the destination buffer.
 * @returns - The total size of the string representation, regardless of how
 *          many characters were written to the destination buffer. */
size_t _gid_enum_param_get_current_value_string(
  const void* data,
  char* dst,
  size_t dstSize)
{
  const GIDEnumParamData* paramData = data;
  const char* fallback = "[Unknown Type]";
  switch(paramData->type)
  {
    case GID_ENUM_PARAM_TYPE_INT64:
      return snprintf(
        dst,
        dstSize,
        "%"PRIi64,
        ((int64_t*)paramData->values)[paramData->index]);
    case GID_ENUM_PARAM_TYPE_UINT64:
      return snprintf(
        dst,
        dstSize,
        "%"PRIu64,
        ((uint64_t*)paramData->values)[paramData->index]);
    case GID_ENUM_PARAM_TYPE_STRING:
      return snprintf(
        dst,
        dstSize,
        "\"%s\"",
        ((char**)paramData->values)[paramData->index]);
    default:
      strncpy(dst, fallback, dstSize);
      return strlen(fallback);
  }
}

/* Moves the value of an 'enum parameter' to the next value that was defined
 * by the test, if any.
 * @param data - Pointer to the GIDEnumParamData.
 * @returns - Non-zero if a 'next value' was selected. Zero means that there
 *          are no more 'next' values. */
int _gid_enum_param_next_value(void* data)
{
  GIDEnumParamData* paramData = data;
  if(paramData->index + 1 < paramData->count)
  {
    paramData->index++;
    return 1;
  }
  else
  {
    return 0;
  }
}

/* Resets an 'enum parameter' to the first value defined by the test.
 * @param data - Pointer to the GIDEnumParamData. */
void _gid_enum_param_reset_value(void* data)
{
  GIDEnumParamData* paramData = data;
  paramData->index = 0;
}

/* Frees memory allocated for an 'enum parameter'.
 * @param data - Pointer to the GIDEnumParamData. */
void _gid_enum_param_free_data(void* data)
{
  const GIDEnumParamData* paramData = data;
  free(paramData->values);
  free(data);
}

/* Creates an 'enum parameter'.
 * @param name - The name of the parameter.
 * @param type - The GIDEnumParamType that defines the type of values that
 *        will be stored in the parameter.
 * @param values - Pointer to the array of values. These values will be copied.
 * @param valueSize - The size of each value.
 * @param valueCount - The number of values.
 * @returns - Pointer to the GIDParamBase for the allocated enum parameter. */
GIDParamBase* _gid_create_enum_param(
  const char* name,
  GIDEnumParamType type,
  void* values,
  size_t valueSize,
  size_t valueCount)
{
  GIDEnumParamData* paramData = malloc(sizeof(GIDEnumParamData));
  paramData->type = type;
  paramData->values = malloc(valueSize * valueCount);
  memcpy(paramData->values, values, valueSize * valueCount);
  paramData->value_size = valueSize;
  paramData->count = valueCount;
  paramData->index = 0;

  GIDParamBase* base = malloc(sizeof(GIDParamBase));
  base->data = paramData;
  base->name = _gid_strclone(name);
  base->value_count = _gid_enum_param_value_count;
  base->current_value = _gid_enum_param_get_current_value;
  base->current_value_string = _gid_enum_param_get_current_value_string;
  base->next_value = _gid_enum_param_next_value;
  base->reset_value = _gid_enum_param_reset_value;
  base->free_data = _gid_enum_param_free_data;
  base->next = NULL;
  return base;
}

/* Defines the status of a test in general (not for a specific
 * configuration). */
typedef enum GIDTestStatus
{
  /* The test has not yet started. */
  GID_TEST_PENDING,

  /* The test is currently running. This includes setup and
   * teardown. */
  GID_TEST_RUNNING,

  /* The test has failed. Note that this value is assigned immediately
   * when one test configuration fails, but other configurations may still
   * be pending. */
  GID_TEST_FAILED,

  /* The test has passed with no failures. */
  GID_TEST_PASSED,
} GIDTestStatus;

/* Defines the steps in a test run. */
typedef enum GIDTestStep
{
  /* The setup for the test is being run. */
  GID_STEP_SETUP = 0,

  /* The test is being run. */
  GID_STEP_RUN = 1,

  /* The test is in the 'teardown' step. */
  GID_STEP_TEARDOWN = 2,
} GIDTestStep;

/* Contains information about a test failure. */
typedef struct GIDTestFailure
{
  /* The name of the test that failed. */
  const char* test_name;

  /* The configuration of the test that resulted in this failure.
   * This is a string representation of all of the test's parameters. */
  const char* configuration;

  /* A message that explains why the test failed. */
  const char* message;

  /* The source file in which the failure was located. */
  const char* source_file;

  /* The line in the source file where the failure was detected. */
  int line;

  /* The GIDTestStep that defines what step resulted in this failure.
   * For example, some tests may fail in setup. */
  GIDTestStep step;

  /* Pointer to the next GIDTestFailure, if any. */
  struct GIDTestFailure* next;

} GIDTestFailure;

/* Creates a GIDTestFailure.
 * @param testName - The name of the test that failed.
 * @param config - The configuration of the test that resulted in the failure.
 *        This is a string representation of all of the test's parameters.
 * @param message - Message that explains why the test failed.
 * @param srcFile - The source file in which the failure was detected.
 * @param line - The line in the source file where the failure was detected.
 * @param step - The GIDTestStep that defines what step resulted in the failure.
 * @returns - A pointer to the allocated GIDTestFailure. */
GIDTestFailure* _gid_create_test_failure(
  const char* testName,
  const char* config,
  const char* message,
  const char* srcFile,
  int line,
  GIDTestStep step)
{
  GIDTestFailure* ret = malloc(sizeof(GIDTestFailure));
  ret->test_name = _gid_strclone(testName);
  ret->configuration = _gid_strclone(config);
  ret->message = _gid_strclone(message);
  ret->source_file = _gid_strclone(srcFile);
  ret->line = line;
  ret->step = step;
  ret->next = NULL;
  return ret;
}

/* Frees a linked list of GIDTestFailures.
 * @param root - Pointer to the root GIDTestFailure to free. */
void _gid_free_test_failures(GIDTestFailure* root)
{
  while(root != NULL)
  {
    GIDTestFailure* next = root->next;
    free((char*)root->test_name);
    free((char*)root->configuration);
    free((char*)root->message);
    free((char*)root->source_file);
    free(root);
    root = next;
  }
}

/* Contains information about a test. */
typedef struct GIDTest
{
  /* The name of the test. */
  const char* name;

  /* Pointer to the GIDParamBase of the first parameter in this test. */
  GIDParamBase* first_param;

  /* Pointer to the GIDParamBase of the last parameter in this test. */
  GIDParamBase* last_param;

  /* Pointer to the next linked GIDTest, or NULL. */
  struct GIDTest* next;

  /* The total number of configurations (combinations of all parameters)
   * for this test. */
  int32_t total_config_count;

  /* The number of configurations that passed. */
  int32_t pass_config_count;

  /* The number of configurations that were run. */
  int32_t run_config_count;

  /* The number of configurations that were skipped. */
  int32_t skip_config_count;

  /* The current status of this test. */
  GIDTestStatus status;

  /* The total amount of time to execute all configurations of this test,
   * measured in milliseconds. */
  int32_t total_runtime;

  /* Have all configurations of this test been run? */
  int is_complete;

  /* Pointer to the GIDTestFailure of the first failure, or NULL. */
  GIDTestFailure* first_failure;

  /* Pointer to the GIDTestFailure of the last failure, or NULL. */
  GIDTestFailure* last_failure;

} GIDTest;

/* Creates a GIDTest.
 * @param name - The name of the test.
 * @returns - A pointer to the allocated GIDTest. */
GIDTest* _gid_create_test(const char* name)
{
  GIDTest* test = malloc(sizeof(GIDTest));
  test->name = _gid_strclone(name);
  test->first_param = NULL;
  test->last_param = NULL;
  test->next = NULL;
  test->total_config_count = 1;
  test->pass_config_count = 0;
  test->run_config_count = 0;
  test->skip_config_count = 0;
  test->status = GID_TEST_PENDING;
  test->total_runtime = 0;
  test->is_complete = 0;
  test->first_failure = NULL;
  test->last_failure = NULL;
  return test;
}

/* Adds a parameter to a test.
 * @param test - Pointer to the GIDTest to which to add the parameter.
 * @param param - Pointer to the GIDParamBase of the parameter to add. */
void _gid_add_param(GIDTest* test, GIDParamBase* param)
{
  GIDParamBase* prev = test->last_param;
  test->last_param = param;

  if(test->first_param == NULL)
    test->first_param = param;
  if(prev != NULL)
    prev->next = param;

  size_t valueCount = _gid_param_value_count(param);
  if(valueCount > 0)/* Variables may have 0 values */
    test->total_config_count *= valueCount;
}

/* Adds a failure to a test.
 * @param dst - Pointer to the GIDTest to which to add the failure.
 * @param failure - Pointer to the GIDTestFailure to add. */
void _gid_add_test_failure(GIDTest* dst, GIDTestFailure* failure)
{
  GIDTestFailure* prev = dst->last_failure;
  dst->last_failure = failure;

  if(dst->first_failure == NULL)
    dst->first_failure = failure;
  if(prev != NULL)
    prev->next = failure;
}

/* Resets all parameters, up to a specific limit, in a test to their initial
 * values.
 * @param test - Pointer to the GIDTest on which to reset the parameters.
 * @param limit - Pointer to the GIDParamBase of the first parameter that
 *        will NOT be reset. All parameters after this will also remain
 *        unchanged. */
void _gid_reset_params_before(GIDTest* test, GIDParamBase* limit)
{
  GIDParamBase* cur = test->first_param;
  while(cur != limit)
  {
    _gid_param_reset_value(cur);
    cur = cur->next;
  }
}

/* Performs one cycle of combinations on the parameters for a specific test.
 * @param test - Pointer to the GIDTest on which to cycle the parameters.
 * @returns - Non-zero if there are more combinations through which to cycle,
 *            otherwise zero is returned. */
int _gid_cycle_params(GIDTest* test)
{
  GIDParamBase* cur = test->first_param;
  while(cur != NULL)
  {
    if(_gid_param_next_value(cur))
    {
      //Reset everything before this
      _gid_reset_params_before(test, cur);
      return 1;
    }
    else
    {
      //This param is at max value, try to increment the next
      cur = cur->next;
    }
  }

  //If we made it here then there are no params
  return 0;
}

/* Finds the GIDParamBase of a parameter with a specfiic name.
 * @param test - Pointer to the GIDTest that contains the parameter to find.
 * @param name - The name of the parameter to find.
 * @returns - Pointer to the GIDParamBase of the parameter, or NULL if it
 *          was not found. */
GIDParamBase* _gid_find_param(GIDTest* test, const char* name)
{
  GIDParamBase* cur = test->first_param;
  while(cur != NULL)
  {
    if(strncmp(cur->name, name, strlen(name)+1) == 0)
      return cur;
    else
      cur = cur->next;
  }

  return NULL;
}

/* Defines the result of a run of a specific test configuration. */
typedef enum GIDRunResult
{
  /* The test configuration's result is still pending. */
  GID_RUN_RESULT_PENDING,

  /* The test configuration has resulted in failure. */
  GID_RUN_RESULT_FAILED,

  /* The test configuration was skipped. */
  GID_RUN_RESULT_SKIPPED,

  /* The test configuration was run successfully. */
  GID_RUN_RESULT_PASSED,
} GIDRunResult;

/* Contains information about a run of a specific configuration of a test. */
typedef struct GIDRun
{
  /* String representation of the parameters of this test run.
   * This includes the names and values of each parameter. */
  char configuration[GID_MAX_CONFIGURATION_STRING_LENGTH];

  /* The current run result. */
  GIDRunResult run_result;

  /* The current test step (such as setup) of this run. */
  GIDTestStep step;

  /* The total amount of time that this run took, including
   * setup and teardown, measured in milliseconds. */
  int32_t runtime;
} GIDTestRun;

/* Contains a set of similar tests. */
typedef struct GIDTestSuite
{
  /* The name of the test suite. */
  const char* name;

  /* Pointer to the first GIDTest, or NULL. */
  GIDTest* first_test;

  /* Pointer to the last GIDTest, or NULL. */
  GIDTest* last_test;

  /* Function that runs the test suite, as generated by the
   * BEGIN_TEST_SUITE and END_TEST_SUITE macros. */
  void (*func)();

  /* Pointer to the next GIDTestSuite, if any. */
  struct GIDTestSuite* next;
} GIDTestSuite;

/* Creates a test suite.
 * @param name - The name of the test suite.
 * @param func - The function that was generated by the BEGIN_TEST_SUITE and
 *        END_TEST_SUITE macros.
 * @returns - A pointer to the allocated GIDTestSuite. */
GIDTestSuite* _gid_create_test_suite(const char* name, void (*func)())
{
  GIDTestSuite* suite = malloc(sizeof(GIDTestSuite));
  suite->name = _gid_strclone(name);
  suite->first_test = NULL;
  suite->last_test = NULL;
  suite->func = func;
  suite->next = NULL;
  return suite;
}

/* Pointer to the first GIDTestSuite, or NULL. */
GIDTestSuite* _gid_first_suite = NULL;

/* Pointer to the last GIDTestSuite, or NULL. */
GIDTestSuite* _gid_last_suite = NULL;

/* Frees a test suite.
 * @param suite - Pointer to the GIDTestSuite to free. */
void _gid_free_suite(GIDTestSuite* suite)
{
  GIDTest* cur = suite->first_test;
  while(cur != NULL)
  {
    GIDTest* next = cur->next;
    free((char*)cur->name);
    _gid_free_params(cur->first_param);
    _gid_free_test_failures(cur->first_failure);
    free(cur);
    cur = next;
  }
  free((char*)suite->name);
  free(suite);
}

/* Calculates a statistical summary of a particular test suite.
 * @param suite - Pointer to the GIDTestSuite from which to obtain the summary.
 * @param totalTests - Pointer to an integer that will be assigned to the total
 *        number of tests in the suite (not counting each configuration).
 * @param passTests - Pointer to an integer that will be assigned to the number
 *        of tests in the suite that had no failures under any configuration.
 * @param failTests - Pointer to an integer that will be assigned to the number
 *        of tests that had at least one failure in any configuration.
 * @param totalConfigs - The total number of configurations of all tests.
 * @param passConfigs - The number of passing configurations.
 * @param skipConfigs - The number of configurations that were skipped. */
void _gid_get_suite_summary(
  const GIDTestSuite* suite,
  int32_t* totalTests,
  int32_t* passTests,
  int32_t* failTests,
  int32_t* totalConfigs,
  int32_t* passConfigs,
  int32_t* skipConfigs)
{
  *totalTests = 0;
  *passTests = 0;
  *failTests = 0;
  *totalConfigs = 0;
  *passConfigs = 0;
  *skipConfigs = 0;

  GIDTest* test = suite->first_test;
  while(test != NULL)
  {
    *totalTests = *totalTests + 1;
    if(test->status == GID_TEST_PASSED)
      *passTests = *passTests + 1;
    if(test->status == GID_TEST_FAILED)
      *failTests = *failTests + 1;
    *totalConfigs += test->total_config_count;
    *passConfigs += test->pass_config_count;
    *skipConfigs += test->skip_config_count;
    test = test->next;
  }
}

/* Prints a summary of the test results.
 * @returns - The exit code, where zero means all tests passed, and non-zero
 *          means that at least one test failed. */
int _gid_summary()
{
  int32_t totalSuites = 0;
  int32_t passSuites = 0;
  int32_t failSuites = 0;
  int32_t totalTests = 0;
  int32_t passTests = 0;
  int32_t failTests = 0;
  int32_t totalTestConfigs = 0;
  int32_t passTestConfigs = 0;
  int32_t skipTestConfigs = 0;

  GIDTestSuite* suite = _gid_first_suite;
  while(suite != NULL)
  {
    totalSuites++;
    int32_t cur_totalTests = 0,
      cur_passTests = 0,
      cur_failTests = 0,
      cur_totalConfigs = 0,
      cur_passConfigs = 0,
      cur_skipConfigs = 0;

    _gid_get_suite_summary(
      suite,
      &cur_totalTests,
      &cur_passTests,
      &cur_failTests,
      &cur_totalConfigs,
      &cur_passConfigs,
      &cur_skipConfigs);

    totalTests += cur_totalTests;
    passTests += cur_passTests;
    failTests += cur_failTests;
    totalTestConfigs += cur_totalConfigs;
    passTestConfigs += cur_passConfigs;
    skipTestConfigs += cur_skipConfigs;

    if(cur_failTests > 0)
      failSuites++;
    else
      passSuites++;

    suite = suite->next;
  }

  printf("\n========SUMMARY========\n");
  printf("%i suites tested, %i passed without failures, "
    "and %i had at least one test failure.\n",
    totalSuites,
    passSuites,
    failSuites);

  printf("%i tests total, %i tests passed, and %i failed.\n",
    totalTests,
    passTests,
    failTests);
  printf("%i total test configurations, %i passed, %i failed, and "
    "%i skipped.\n",
    totalTestConfigs,
    passTestConfigs,
    totalTestConfigs-(passTestConfigs+skipTestConfigs),
    skipTestConfigs);

  if(failSuites > 0 || failTests > 0)
  {
    printf("\n\033[30;41m  Failures  \033[0m\n");
    suite = _gid_first_suite;
    while(suite != NULL)
    {
      int32_t cur_totalTests = 0,
        cur_passTests = 0,
        cur_failTests = 0,
        cur_totalConfigs = 0,
        cur_passConfigs = 0,
        cur_skipConfigs = 0;

      _gid_get_suite_summary(
        suite,
        &cur_totalTests,
        &cur_passTests,
        &cur_failTests,
        &cur_totalConfigs,
        &cur_passConfigs,
        &cur_skipConfigs);

      if(cur_failTests > 0
        || (cur_totalConfigs != cur_passConfigs + cur_skipConfigs))
      {
        GIDTest* test = suite->first_test;
        while(test != NULL)
        {
          if(test->first_failure != NULL)
          {
            printf("------------------------------------------------------\n");
            GIDTestFailure* failure = test->first_failure;
            while(failure != NULL)
            {
              const char* step;
              switch(failure->step)
              {
                case GID_STEP_SETUP:
                  step = "\t[Setup]";
                  break;
                case GID_STEP_TEARDOWN:
                  step = "\t[Teardown]";
                  break;
                default:
                  step = "";
                  break;
              }

              printf("%s > %s(%s) Failed\n",
                suite->name,
                test->name,
                failure->configuration);
              printf("  Message:\t\033[31m%s\033[0m\n", failure->message);
              printf("  Location:\t%s @%i%s\n",
                failure->source_file,
                failure->line,
                step);
              printf("\n");
              failure = failure->next;
            }
          }

          test = test->next;
        }
      }
      suite = suite->next;
    }
  }

  return failSuites == 0 ? 0/*No-error*/ : -1/*Fail*/;
}

/* Frees all memory allocated for GID testing. */
void _gid_free()
{
  GIDTestSuite* suite = _gid_first_suite;
  while(suite != NULL)
  {
    GIDTestSuite* next = suite->next;
    _gid_free_suite(suite);
    suite = next;
  }
  _gid_first_suite = NULL;
  _gid_last_suite = NULL;
}

/* Runs all tests defined in each test suite, and prints a summary.
 * @returns - Zero if all tests passed, otherwise non-zero. */
int gidunit()
{
  //Run the test suite functions
  GIDTestSuite* suite = _gid_first_suite;
  while(suite != NULL)
  {
    suite->func();
    suite = suite->next;
  }

  //Generate the summary
  int ret = _gid_summary();
  _gid_free();
  return ret;
}

/* Registers a test suite.
 * @param suite - Pointer to the GIDTestSuite to register. */
void _gid_add_test_suite(GIDTestSuite* suite)
{
  GIDTestSuite* prev = _gid_last_suite;
  _gid_last_suite = suite;

  if(_gid_first_suite == NULL)
    _gid_first_suite = suite;
  if(prev != NULL)
    prev->next = suite;
}

/* Adds a test to a suite.
 * @param dst - Pointer to the destination GIDTestSuite.
 * @param test - Pointer to the GIDTest to add. */
void _gid_add_test(GIDTestSuite* dst, GIDTest* test)
{
  GIDTest* prev = dst->last_test;
  dst->last_test = test;

  if(dst->first_test == NULL)
    dst->first_test = test;
  if(prev != NULL)
    prev->next = test;
}


/* Clears the current line on stdout. */
#define _gid_clear_console_line() printf("%c[2k\r", 27)

/* Prints the current status of a test on the lowest line.
 * This will erase the lowest line then overwrite it. */
void _gid_print_test_status(const GIDTest* test)
{
  const char* statusStr = "?";
  switch(test->status)
  {
    case GID_TEST_PENDING:
      statusStr = "\033[37;40m IDLE \033[0m";
      break;

    case GID_TEST_RUNNING:
      statusStr = "\033[30;43m RUN \033[0m";
      break;

    case GID_TEST_FAILED:
      statusStr = "\033[30;41m FAIL \033[0m";
      break;

    case GID_TEST_PASSED:
      statusStr = "\033[30;42m PASS \033[0m";
      break;
  }


  _gid_clear_console_line();
  printf("%s\t%s\t%i/%i configurations passed",
    statusStr,
    test->name,
    test->pass_config_count,
    test->total_config_count);
  if(test->skip_config_count > 0)
    printf("\t(%i skipped)", test->skip_config_count);
  if(test->is_complete)
  {
    int32_t avgMillis = test->total_runtime / test->run_config_count;
    if(test->total_config_count > 1)
    {
      printf("\t[%ims total, avg %ims per configuration]",
        test->total_runtime,
        avgMillis);
    }
    else
    {
      printf("\t[%ims]", test->total_runtime);
    }
  }
  fflush(stdout);
}

/* Prepares to run a test (before any specific configuration is executed).
 * This will update the status of the test to 'running', and will print the
 * status to stdout.
 * @param test - Pointer to the GIDTest that is being prepared. */
void _gid_pre_test(GIDTest* test)
{
  test->status = GID_TEST_RUNNING;
  _gid_print_test_status(test);
}

/* Finalizes the result of a test after all configurations have been run.
 * This will determine whether the test passed or failed (based on whether
 * any configuration resulted in failure), and will print the status to stdout.
 * @param test - Pointer to the GIDTest that has finished. */
void _gid_post_test(GIDTest* test)
{
  if(test->pass_config_count + test->skip_config_count
      == test->total_config_count)
    test->status = GID_TEST_PASSED;
  else
    test->status = GID_TEST_FAILED;
  test->is_complete = 1;
  _gid_print_test_status(test);
  printf("\n");
}

/* Prepares to run a specific configuration of a test.
 * @param test - Pointer to the GIDTest.
 * @param run - Pointer to the GIDTestRun that is about to run. */
void _gid_pre_test_config_run(GIDTest* test, GIDTestRun* run)
{
  test->run_config_count++;
  _gid_print_test_status(test);
}

/* Finalizes a run of a specific configuration of a test.
 * @param test - Pointer to the GIDTest.
 * @param run - Pointer to the GIDTestRun that has finished. */
void _gid_post_test_config_run(GIDTest* test, GIDTestRun* run)
{
  switch(run->run_result)
  {
    case GID_RUN_RESULT_FAILED:
      test->status = GID_TEST_FAILED;
      break;

    case GID_RUN_RESULT_PENDING:
    case GID_RUN_RESULT_SKIPPED:
      test->skip_config_count++;
      break;

    case GID_RUN_RESULT_PASSED:
      test->pass_config_count++;
      break;
  }

  test->total_runtime += run->runtime;
  _gid_print_test_status(test);
}

/* Structure that contains time tracking data. */
typedef struct GIDTimer
{
#ifdef _WIN32
  /* The start timestamp, based on GetTickCount(). */
  int32_t start;
#else
  /* The time when the timer started. */
  struct timeval start;
#endif
} GIDTimer;

/* Allocates and starts a GIDTimer.
 * @param test - Pointer to the GIDTest to associate with the timer.
 * @returns - Pointer to the allocated GIDTimer. */
GIDTimer* _gid_start_timer(const GIDTest* test)
{
  GIDTimer* ret = malloc(sizeof(GIDTimer));
#ifdef _WIN32
  ret->start = GetTickCount();
#else
  gettimeofday(&ret->start, NULL);
#endif
  return ret;
}

/* Stops a GIDTimer and frees its memory.
 * @param timer - Pointer to the GIDTimer to stop.
 * @returns - The total elapsed time of the timer, in milliseconds. */
int32_t _gid_stop_timer(GIDTimer* timer)
{
#ifdef _WIN32
  int32_t ret = GetTickCount() - timer->start;
#else
  struct timeval end;
  gettimeofday(&end, NULL);
  struct timeval duration;
  timersub(&end, &timer->start, &duration);
  int32_t ret = (duration.tv_sec * 1000) + (duration.tv_usec / 1000);
#endif
  free(timer);
  return ret;
}

/* Skips the current test run, giving it no result (neither fail nor pass).
 * Use this if, for example, it is not possible to test a particular test
 * configuration. */
#define skip() goto _GID_TEST_END

/* Cause the current test run to fail with a specific formatted message.
 * @param message - The formatted message that explains the failure.
 * @param ... - Values of the variables in the formatted message. */
#define assert_fail_format(message, ...)                                      \
{                                                                             \
  _gid_cur_run.run_result = GID_RUN_RESULT_FAILED;                            \
  char _gidMsgBuf[GID_MAX_MESSAGE_LENGTH];                                    \
  snprintf(_gidMsgBuf, GID_MAX_MESSAGE_LENGTH, (message), __VA_ARGS__);       \
  GIDTestFailure* _gidFailure = _gid_create_test_failure(                     \
    _gid_cur_test->name,                                                      \
    _gid_cur_run.configuration,                                               \
    _gidMsgBuf,                                                               \
    __FILE__,                                                                 \
    __LINE__,                                                                 \
    _gid_cur_run.step);                                                       \
  _gid_add_test_failure(_gid_cur_test, _gidFailure);                          \
  goto _GID_TEST_END;                                                         \
}

/* Causes the current test run to fail with a specific message.
 * @param message - The message that explains the failure. */
#define assert_fail(message) assert_fail_format(message"%s", "")

/* Asserts that a condition is true, and prints a specific formatted
 * message upon failure.
 * @param condition - The condition that must evaluate to non-zero for
 *        the test to pass.
 * @param msg - The formatted message to print when the assertion fails.
 * @param ... - Values of the variables in the formatted message. */
#define assert_message_format(condition, msg, ...)                            \
  if(!(condition))                                                            \
    assert_fail_format(msg, __VA_ARGS__);

/* Asserts that a condition is true, and prints a specific message upon
 * failure.
 * @param condition - The condition that must evaluate to non-zero for
 *        the test to pass.
 * @param msg - The message to print when the assertion fails. */
#define assert_message(condition, msg)                                        \
  assert_message_format(condition, msg"%s", "")

/* Asserts that an expression evaluates to true (non-zero).
 * @param condition - The condition to evaluate. */
#define assert(condition)                                                     \
  assert_message(                                                             \
    condition,                                                                \
    "The expression '"#condition"' evaluated to false.")

/* Asserts that a signed integer is equal to an expected value.
 * @param expected - The expected value.
 * @param actual - The actual value. */
#define assert_int_eq(expected, actual)                                       \
  {                                                                           \
    int64_t _gidExpVal = (expected);                                          \
    int64_t _gidActVal = (actual);                                            \
    assert_message_format(                                                    \
      _gidExpVal == _gidActVal,                                               \
      "Expected '"#actual"' to evaluate to %"PRId64                           \
      ", but instead it evaluated to %"PRId64".",                             \
      _gidExpVal,                                                             \
      _gidActVal);                                                            \
  }

/* Asserts that a signed integer is not equal to a specific value.
 * @param expected - The unexpected value.
 * @param actual - The actual value. */
#define assert_int_not_eq(unexpected, actual)                                 \
  {                                                                           \
    int64_t _gidExpVal = (unexpected);                                        \
    int64_t _gidActVal = (actual);                                            \
    assert_message_format(                                                    \
      _gidExpVal != _gidActVal,                                               \
      "'"#actual"' evaluated to the unexpected value %"PRId64".",             \
      _gidExpVal);                                                            \
  }

/* Asserts that an unsigned integer is equal to an expected value.
 * @param expected - The expected value.
 * @param actual - The actual value. */
#define assert_uint_eq(expected, actual)                                      \
  {                                                                           \
    uint64_t _gidExpVal = (expected);                                         \
    uint64_t _gidActVal = (actual);                                           \
    assert_message_format(                                                    \
      _gidExpVal == _gidActVal,                                               \
      "Expected '"#actual"' to evaluate to %"PRIu64                           \
      ", but instead it evaluated to %"PRIu64".",                             \
      _gidExpVal,                                                             \
      _gidActVal);                                                            \
  }

/* Asserts that an unsigned integer is not equal to a specific value.
 * @param expected - The unexpected value.
 * @param actual - The actual value. */
#define assert_uint_not_eq(unexpected, actual)                                \
  {                                                                           \
    uint64_t _gidExpVal = (unexpected);                                       \
    uint64_t _gidActVal = (actual);                                           \
    assert_message_format(                                                    \
      _gidExpVal != _gidActVal,                                               \
      "'"#actual"' evaluated to the unexpected value %"PRIu64".",             \
      _gidExpVal);                                                            \
  }

/* Asserts that a pointer is equal to an expected value.
 * @param expected - The expected value.
 * @param actual - The actual value. */
#define assert_pointer_eq(expected, actual)                                   \
  {                                                                           \
    const void* _gidExpVal = (expected);                                      \
    const void* _gidActVal = (actual);                                        \
    assert_message_format(                                                    \
      _gidExpVal == _gidActVal,                                               \
      "Expected '"#actual"' to evaluate to %p"                                \
      ", but instead it evaluated to %p.",                                    \
      _gidExpVal,                                                             \
      _gidActVal);                                                            \
  }

/* Asserts that a pointer is not equal to a specific value.
 * @param expected - The unexpected value.
 * @param actual - The actual value. */
#define assert_pointer_not_eq(unexpected, actual)                             \
  {                                                                           \
    const void* _gidExpVal = (unexpected);                                    \
    const void* _gidActVal = (actual);                                        \
    assert_message_format(                                                    \
      _gidExpVal != _gidActVal,                                               \
      "'"#actual"' evaluated to the unexpected value %p.",                    \
      _gidExpVal);                                                            \
  }

/* Asserts that a pointer is NULL.
 * @param pointer - The pointer. */
#define assert_null(pointer)                                                  \
  {                                                                           \
    const void* _gidActual = (pointer);                                       \
    assert_message_format(                                                    \
      _gidActual == NULL,                                                     \
      "'"#pointer"' was %p, not NULL.",                                       \
      _gidActual);                                                            \
  }

/* Asserts that a pointer is anything except NULL.
 * @param pointer - The pointer. */
#define assert_not_null(pointer)                                              \
  {                                                                           \
    const void* _gidActual = (pointer);                                       \
    assert_message(_gidActual != NULL, "'"#pointer"' was NULL.");             \
  }

/* Asserts that a null-terminated string is equal to an expected value.
 * @param expected - The expected value.
 * @param actual - The actual value. */
#define assert_string_eq(expected, actual)                                    \
  {                                                                           \
    const char* _gidExpVal = (char*)(expected);                               \
    const char* _gidActVal = (char*)(actual);                                 \
    assert_message_format(                                                    \
      strcmp(_gidExpVal, _gidActVal)==0,                                      \
      "Expected "#actual" to be equal to \"%s\""                              \
      ", but instead it was \"%s\".",                                         \
      _gidExpVal,                                                             \
      _gidActVal);                                                            \
  }

/* Asserts that a null-terminated string is not equal to a specific value.
 * @param expected - The unexpected value.
 * @param actual - The actual value. */
#define assert_string_not_eq(unexpected, actual)                              \
  {                                                                           \
    const char* _gidExpVal = (char*)(unexpected);                             \
    const char* _gidActVal = (char*)(actual);                                 \
    assert_message_format(                                                    \
      strcmp(_gidExpVal, _gidActVal)!=0,                                      \
      #actual" was equal to the unexpected value \"%s\".",                    \
      _gidActVal);                                                            \
  }

/* Asserts that a floating point value is equal to an expected value.
 * @param expected - The expected value.
 * @param actual - The actual value. */
#define assert_double_eq(expected, actual)                                    \
  {                                                                           \
    double _gidExpVal = (expected);                                           \
    double _gidActVal = (actual);                                             \
    assert_message_format(                                                    \
      _gidExpVal == _gidActVal,                                               \
      "Expected '"#actual"' to evaluate to %f"                                \
      ", but instead it evaluated to %f.",                                    \
      _gidExpVal,                                                             \
      _gidActVal);                                                            \
  }

/* Asserts that a floating point value is not equal to a specific value.
 * @param expected - The unexpected value.
 * @param actual - The actual value. */
#define assert_double_not_eq(unexpected, actual)                              \
  {                                                                           \
    double _gidExpVal = (unexpected);                                         \
    double _gidActVal = (actual);                                             \
    assert_message_format(                                                    \
      _gidExpVal != _gidActVal,                                               \
      "'"#actual"' evaluated to the unexpected value %f.",                    \
      _gidActVal);                                                            \
  }

/* Asserts that one portion of memory is equal to another.
 * @param expected - Pointer to the first byte of the expected memory value.
 * @param actual - Pointer to the first byte of the actual memory value.
 * @param size - The number of bytes to compare. */
#define assert_memory_eq(expected, actual, size)                              \
  {                                                                           \
    const uint8_t* _gidExpBuf = (const uint8_t*)(expected);                   \
    const uint8_t* _gidActBuf = (const uint8_t*)(actual);                     \
    size_t _gidCmpSize = (size);                                              \
    size_t _gidFirstDiff = _gid_cmp_memory(                                   \
      _gidExpBuf,                                                             \
      _gidActBuf,                                                             \
      _gidCmpSize);                                                           \
    assert_message_format(                                                    \
      _gidFirstDiff == _gidCmpSize,                                           \
      "Expected "#actual" to have the same %"PRIu64" bytes as "               \
      #expected", but "#actual"[%"PRIu64"]=0x%x and "#expected"[%"            \
      PRIu64"]=0x%x.",                                                        \
      (uint64_t)_gidCmpSize,                                                  \
      (uint64_t)_gidFirstDiff,                                                \
      _gidActBuf[_gidFirstDiff],                                              \
      (uint64_t)_gidFirstDiff,                                                \
      _gidExpBuf[_gidFirstDiff]);                                             \
  }

/* Asserts that one portion of memory is not equal to another.
 * @param unexpected - Pointer to the first byte of the unexpected memory.
 * @param actual - Pointer to the first byte of the actual memory.
 * @param size - The number of bytes to compare. */
#define assert_memory_not_eq(unexpected, actual, size)                        \
  {                                                                           \
    const uint8_t* _gidExpBuf = (const uint8_t*)(unexpected);                 \
    const uint8_t* _gidActBuf = (const uint8_t*)(actual);                     \
    size_t _gidCmpSize = (size);                                              \
    size_t _gidFirstDiff = _gid_cmp_memory(                                   \
      _gidExpBuf,                                                             \
      _gidActBuf,                                                             \
      _gidCmpSize);                                                           \
    assert_message(                                                           \
      _gidFirstDiff != _gidCmpSize,                                           \
      #actual" has the same exact memory as "#unexpected".");                 \
  }


/* Internal helper macro that allows an optional semicolon at the end. */
#define _gid_allow_optional_semicolon() {}

/* Macro that reads a parameter into a variable.
 * @param type - The type of the variable.
 * @param var_name - The name of the variable. */
#define _gid_read_variable(type, var_name)                                    \
          {                                                                   \
            type* _gidValPtr;                                                 \
            GIDParamBase* _gidParam = _gid_find_param(                        \
              _gid_cur_test,                                                  \
              #var_name);                                                     \
            _gidValPtr = _gid_param_get_value(_gidParam);                     \
            if(_gidValPtr != NULL)                                            \
              ((type*)&var_name)[0] = (*_gidValPtr);                          \
            else                                                              \
              ((type*)&var_name)[0] = (type)NULL;                             \
          }

/* Checks whether the scope containing this line belongs to the test that is
 * currently being executed. This is important because all tests exist in the
 * 'main test loop' at once, but each test has its own local scope. If you
 * didn't check this condition (or other similar conditions), then your code
 * would run for every step of every test configuration.  */
#define _gid_is_this_test_running (_gid_cur_test != NULL                      \
          && _gid_test_step == GID_STEP_RUN                                   \
          && strncmp(_gid_cur_test->name,                                     \
               _gid_scope_test_name,                                          \
               strlen(_gid_scope_test_name)+1) == 0)

/* Checks if initialization is in progress. Initialization is when 'test
 * registration' happens, when _gid_cur_test is NULL and all of the user-
 * defined tests, parameters, and suites are being registered. This happens
 * during the first iteration of the 'main test loop'. */
#define _gid_is_initializing (_gid_cur_test == NULL)

/* Ends the scope of a previous test. */
#define _gid_end_previous_test_scope                                          \
        if(_gid_is_this_test_running                                          \
           && _gid_cur_run.run_result == GID_RUN_RESULT_PENDING)              \
          _gid_cur_run.run_result = GID_RUN_RESULT_PASSED;                    \
      }                                                                       \
      while(0);                                                               \
      if(_gid_is_this_test_running                                            \
         && _gid_cur_run.run_result == GID_RUN_RESULT_PENDING)                \
        _gid_cur_run.run_result = GID_RUN_RESULT_SKIPPED;                     \

/* Starts the scope of a new test. */
#define _gid_start_next_test_scope(test_name)                                 \
      do                                                                      \
      {                                                                       \
        _gid_scope_test_name = #test_name;

/* Registers a test suite that you defined by the BEGIN_TEST_SUITE and
 * END_TEST_SUITE macros.
 * @param suite_name - The name of the suite that you created by
 *        BEGIN_TEST_SUITE. This is not a string. */
#define ADD_TEST_SUITE(suite_name)                                            \
  _gid_test_suite_##suite_name = _gid_create_test_suite(                      \
    #suite_name,                                                              \
    _gid_test_suite_func_##suite_name);                                       \
  _gid_add_test_suite(_gid_test_suite_##suite_name)


/* Starts defining a test suite.
 * @param suite_name - The name of the test suite. Must be valid for a function
 *        name. This is not a string.
 * @remarks - Define each test suit outside of any function (as it internally
 *          generates a function). Start by writing the BEGIN_TEST_SUITE macro
 *          with the name of the test suite that you are defining. Then write
 *          the content of the test suite, which consists of multiple Tests
 *          as well as one optional SetUp and TearDown function (see the
 *          respective macros). End the test suite with the END_TEST_SUITE macro.
 *          Do not put additional C code, such as regular  C functions, between
 *          BEGIN_TEST_SUITE and END_TEST_SUITE, since this would actually add a
 *          C function within a function, so you would get compiler errors. Note
 *          that you must register the test suite somewhere in your program, such
 *          as in the main function, via the ADD_TEST_SUITE macro. If you don't do
 *          this, the test suite will not be run when you call the gidunit() main
 *          function.
 * @example -
 *
 * #include "gidunit.h"
 *
 * BEGIN_TEST_SUITE(MyTestSuite)
 *
 *   SetUp()
 *   {
 *     fixture = malloc(sizeof(int));
 *     *((int*)fixture) = 1;
 *   }
 *
 *   TearDown()
 *   {
 *     free(fixture);
 *   }
 *
 *   Test(MyFirstTest)
 *   {
 *     assert_int_eq(1, *(int*)fixture);
 *   }
 *
 *   Test(MySecondTest)
 *   {
 *     assert_int_not_eq(0, *(int*)fixture);
 *   }
 *
 *   Test(MyThirdTest)
 *   {
 *     assert_int_eq(0, 0);
 *   }
 *
 * END_TEST_SUITE()
 *
 * int main()
 * {
 *   ADD_TEST_SUITE(MyTestSuite);
 *   return gidunit();
 * }
 *
 * */
#define BEGIN_TEST_SUITE(suite_name)                                          \
GIDTestSuite* _gid_test_suite_##suite_name = NULL;                            \
void _gid_test_suite_func_##suite_name()                                      \
{                                                                             \
  GIDTestSuite* _gid_test_suite = _gid_test_suite_##suite_name;               \
                                                                              \
  GIDTest* _gid_cur_test = NULL;                                              \
  GIDTest* _gid_added_test = NULL;                                            \
  char* _gid_scope_test_name = NULL;                                          \
  do                                                                          \
  {                                                                           \
    if(_gid_cur_test != NULL)                                                 \
      _gid_pre_test(_gid_cur_test);                                           \
                                                                              \
    do                                                                        \
    {                                                                         \
      void* fixture = NULL;                                                   \
      (void)fixture;                                                          \
      GIDTestRun _gid_cur_run = (GIDTestRun)                                  \
      {                                                                       \
        .run_result = GID_RUN_RESULT_PENDING,                                 \
        .step = GID_STEP_SETUP,                                               \
        .runtime = -1,                                                        \
      };                                                                      \
      GIDTimer* _gid_cur_timer = NULL;                                        \
      if(_gid_cur_test != NULL)                                               \
      {                                                                       \
        _gid_get_params_string(                                               \
          _gid_cur_test->first_param,                                         \
          _gid_cur_run.configuration,                                         \
          GID_MAX_CONFIGURATION_STRING_LENGTH);                               \
        _gid_cur_timer = _gid_start_timer(_gid_cur_test);                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        _gid_cur_run.configuration[0] = '\0';                                 \
      }                                                                       \
      for(GIDTestStep _gid_test_step = GID_STEP_SETUP;                        \
        _gid_test_step <= GID_STEP_TEARDOWN;                                  \
        _gid_test_step++)                                                     \
      {                                                                       \
        _gid_cur_run.step = _gid_test_step;                                   \
        if(_gid_test_step == GID_STEP_RUN)                                    \
          _gid_pre_test_config_run(_gid_cur_test, &_gid_cur_run);             \
        _gid_start_next_test_scope(""/*Start of 'dummy test' scope*/)

/* Defines a setup function for all tests in a test suite.
 * @remarks - You can have at most one SetUp function per test suite. The
 *          SetUp function must be placed between the BEGIN_TEST_SUITE and
 *          END_TEST_SUITE macros. It can be placed above, below, or in between
 *          the various Tests of the suite. The body should be contained by
 *          scope brackets {} immediately following this macro. The SetUp
 *          function will be executed before each test configuration is run.
 *          You may have assert statements inside of the SetUp function,
 *          and any failures will be associated with the proper test.
 *          You will likely need to pass data to the test function, you
 *          can use the 'void* fixture' variable for this purpose. */
#define SetUp()                                                               \
        if(_gid_cur_test != NULL && _gid_test_step == GID_STEP_SETUP)
        /* SetUp body goes here */

/* Defines a test function.
 * @param test_name - The name that you want to assign to this test.
 * @param ... - Definitions of parameters that you want to add to this test.
 *        Each parameter may have multiple values, and the test will be run
 *        with all combinations of all parameter values. Possible parameter
 *        macros are: IntRow, UIntRow, StringRow, RangeParam,
 *        UnsignedRangeParam, EnumParam, UnsignedEnumParam, StringEnumParam.
 * @remarks - Each Test function must be defined between the BEGIN_TEST_SUITE
 *          and END_TEST_SUITE macros. The body of the test function should be
 *          contained by scope brackets {} immediately following this Test
 *          macro. The test body should consist of various assert statements,
 *          such as assert_int_eq. You may also skip a particular run of a
 *          test by using 'skip()'.
 * @example -
 *
 *          Test(MyTest,
 *            RangeParam(i, 5, 10)
 *            EnumParam(j, 2, 4, 8, 16, 32))
 *          {
 *            //This test will be run 6*5=30 times to test each combination
 *            //of variables.
 *
 *            if(j == 8)
 *              skip();
 *            assert_int_eq(5, i);
 *            assert_int_not_eq(32, j);
 *          }
 * */
#define Test(test_name, ...)                                                  \
        _gid_end_previous_test_scope                                          \
        void* gid_test_##test_name = NULL;/*Unused var to force unique names*/\
        (void)gid_test_##test_name;                                           \
        _gid_start_next_test_scope(test_name)                                 \
        GIDParamBase* _gid_int_row_params = NULL;                             \
        GIDParamBase* _gid_uint_row_params = NULL;                            \
        GIDParamBase* _gid_string_row_params = NULL;                          \
        const int64_t* int_row = NULL;                                        \
        const uint64_t* uint_row = NULL;                                      \
        const char** string_row = NULL;                                       \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          _gid_added_test = _gid_create_test(#test_name);                     \
          _gid_add_test(_gid_test_suite, _gid_added_test);                    \
          _gid_int_row_params =                                               \
            _gid_create_row_param("int_row", GID_ROW_PARAM_TYPE_INT64);       \
          _gid_uint_row_params =                                              \
            _gid_create_row_param("uint_row", GID_ROW_PARAM_TYPE_UINT64);     \
          _gid_string_row_params =                                            \
            _gid_create_row_param("string_row", GID_ROW_PARAM_TYPE_STRING);   \
        }                                                                     \
        else if(_gid_is_this_test_running)                                    \
        {                                                                     \
          _gid_read_variable(int64_t*, int_row);                              \
          _gid_read_variable(uint64_t*, uint_row);                            \
          _gid_read_variable(char**, string_row);                             \
        }                                                                     \
        __VA_ARGS__                                                           \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          /*This has to go AFTER __VA_ARGS__ for row counts */                \
          _gid_add_param(_gid_added_test, _gid_int_row_params);               \
          _gid_add_param(_gid_added_test, _gid_uint_row_params);              \
          _gid_add_param(_gid_added_test, _gid_string_row_params);            \
        }                                                                     \
        if(_gid_is_this_test_running)
        /* Test body goes here */

/* Defines a TearDown function for all tests in a particular test suite.
 * @remarks - This TearDown macro must be placed between the
 *          BEGIN_TEST_SUITE and END_TEST_SUITE macros. The body of this
 *          function should be contained by scope brackets {} immediately
 *          following this macro. You may access the 'void* fixture' variable
 *          inside of the TearDown function. */
#define TearDown()                                                            \
        if(_gid_cur_test != NULL && _gid_test_step == GID_STEP_TEARDOWN)
        /* Teardown body goes here */

/* Marks the end of a test suite. */
#define END_TEST_SUITE()                                                      \
        _gid_end_previous_test_scope                                          \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          break;/*For init, break on setup*/                                  \
        }                                                                     \
        else if(_gid_test_step == GID_STEP_TEARDOWN)                          \
        {                                                                     \
          _gid_cur_run.runtime = _gid_stop_timer(_gid_cur_timer);             \
          _gid_post_test_config_run(_gid_cur_test, &_gid_cur_run);            \
        }                                                                     \
        _GID_TEST_END:                                                        \
        continue;                                                             \
      }                                                                       \
    }                                                                         \
    while(_gid_cur_test != NULL &&_gid_cycle_params(_gid_cur_test));          \
    if(_gid_cur_test == NULL)                                                 \
    {                                                                         \
      _gid_cur_test = _gid_test_suite->first_test;                            \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      _gid_post_test(_gid_cur_test);                                          \
      _gid_cur_test = _gid_cur_test->next;                                    \
    }                                                                         \
  }                                                                           \
  while(_gid_cur_test != NULL);                                               \
}

/* Defines a row of integer values to pass to the test.
 * Each time the test is run, it will have an 'int64_t* int_row' variable
 * that contains one row of values defined by this macro.
 * You may (and probably should) define multiple IntRows to run your
 * test with a set of different values.
 * @remarks - Note that each test is run with all combinations of parameter
 *          values, and each IntRow counts as one value (since the parameter
 *          is an array).
 * @example -
 *
 * Test(MyTestFunc,
 *   IntRow(1, 2, 3, 4)
 *   IntRow(2, 4, 6, 8)
 *   IntRow(3, 6, 9, 12))
 *  {
 *    //This test will run three times:
 *    //once with int_row={1,2,3,4},
 *    //then with int_row={2,4,6,8},
 *    //and again with int_row={3,6,9,12}.
 *
 *    assert_not_eq(4, int_row[1]);
 *    assert_eq(2, int_row[0]);
 *  }
 *
 *  */
#define IntRow(...)                                                           \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          int64_t _gidValues[] = {__VA_ARGS__};                               \
          size_t _gidValueCount = sizeof(_gidValues)/sizeof(int64_t);         \
          GIDRowParamValue* _gidRow = _gid_create_row_param_value(            \
            _gidValues,                                                       \
            sizeof(int64_t),                                                  \
            _gidValueCount,                                                   \
            GID_ROW_PARAM_TYPE_INT64);                                        \
          _gid_add_row_param_value(_gid_int_row_params, _gidRow);             \
        }

/* Defines a row of unsigned integer values to pass to the test.
 * This is the same as 'IntRow', except that the values are unsigned and
 * the local variable is 'uint64_t* uint_row'. */
#define UIntRow(...)                                                          \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          uint64_t _gidValues[] = {__VA_ARGS__};                              \
          size_t _gidValueCount = sizeof(_gidValues)/sizeof(uint64_t);        \
          GIDRowParamValue* _gidRow = _gid_create_row_param_value(            \
            _gidValues,                                                       \
            sizeof(uint64_t),                                                 \
            _gidValueCount,                                                   \
            GID_ROW_PARAM_TYPE_UINT64);                                       \
          _gid_add_row_param_value(_gid_uint_row_params, _gidRow);            \
        }

/* Defines a row of string values to pass to the test.
 * This is the same as 'IntRow', except that the values are null-terminated
 * strings and the local variable is 'char** string_row'.
 * @example -
 *
 * Test(MyTestFunc,
 *   StringRow("Alpha", "Bravo", "Charlie")
 *   StringRow("alpha", "bravo", "charlie")
 *   StringRow("ALPHA", "BRAVO", "CHARLIE"))
 * {
 *   if(strcmp(string_row[1], "bravo") == 0)
 *     skip();
 *
 *   assert_string_eq("Alpha", string_row[0]);
 * }
 *
 * */
#define StringRow(...)                                                        \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          char* _gidValues[] = {__VA_ARGS__};                                 \
          size_t _gidValueCount = sizeof(_gidValues)/sizeof(char*);           \
          GIDRowParamValue* _gidRow = _gid_create_row_param_value(            \
            _gidValues,                                                       \
            sizeof(char*),                                                    \
            _gidValueCount,                                                   \
            GID_ROW_PARAM_TYPE_STRING);                                       \
          _gid_add_row_param_value(_gid_string_row_params, _gidRow);          \
        }

/* Defines a parameter variable that will be tested with a range of signed
 * integer values.
 * @param var_name - The name that you want to assign to the local int64_t
 *        variable.
 * @param min_val - The minimum value of the variable.
 * @param max_val - The maximum value of the variable.
 * @remarks - The min_val must not be greater than max_val.
 * @example -
 *
 * Test(MyTestFunc,
 *   RangeParam(i, 0, 3)
 *   RangeParam(j, 4, 6)
 * {
 *   //This test will be run 4*3=12 times, so each combination of values
 *   //can be tested.
 *
 *   assert_int_not_eq(0, i);
 *   assert_int_not_eq(5, j);
 * }
 *
 * */
#define RangeParam(var_name, min_val, max_val)                                \
        int64_t var_name;                                                     \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          int64_t _gidVmin = (int64_t)min_val;                                \
          int64_t _gidVmax = (int64_t)max_val;                                \
          GIDParamBase* _gidParam = _gid_create_range_param(                  \
            #var_name,                                                        \
            _gidVmin,                                                         \
            _gidVmax, 1);                                                     \
          _gid_add_param(_gid_added_test, _gidParam);                         \
        }                                                                     \
        else if(_gid_is_this_test_running)                                    \
        {                                                                     \
          _gid_read_variable(int64_t, var_name);                              \
        }

/* Defines a parameter variable that will be tested with a range of unsigned
 * integer values. This is the same as 'RangeParam', except that the variable
 * is unsigned rather than signed.
 * @param var_name - The name that you want to assign to the local uint64_t
 *        variable
 * @param min_val - The minimum value of the variable.
 * @param max_val - The maximum value of the variable. */
#define UnsignedRangeParam(var_name, min_val, max_val)                        \
        uint64_t var_name;                                                    \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          uint64_t _gidVmin = (uint64_t)min_val;                              \
          uint64_t _gidVmax = (uint64_t)max_val;                              \
          GIDParamBase* _gidParam = _gid_create_range_param(                  \
            #var_name,                                                        \
            _gidVmin,                                                         \
            _gidVmax,                                                         \
            0);                                                               \
          _gid_add_param(_gid_added_test, _gidParam);                         \
        }                                                                     \
        else if(_gid_is_this_test_running)                                    \
        {                                                                     \
          _gid_read_variable(uint64_t, var_name);                             \
        }

/* Defines a parameter variable that will be tested with multiple predefined
 * signed integer values, one at a time.
 * @param var_name - The name that you want to assign to the local int64_t
 *        variable.
 * @param ... - The set of values with which you want the variable to be
 *            tested.
 * @example -
 *
 * Test(MyTestFunc,
 *   EnumParam(i, 0, 1, 2, 4, 8, 16, 32)
 *   EnumParam(j, 5, 10, 15)
 * {
 *   //This test will be run 7*3=21 times, so each combination of values
 *   //can be tested.
 *
 *   assert_int_not_eq(15, j);
 * }
 *
 * */
#define EnumParam(var_name, ...)                                              \
        int64_t var_name;                                                     \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          int64_t _gidValues[] = {__VA_ARGS__};                               \
          size_t _gidCount = sizeof(_gidValues)/sizeof(_gidValues[0]);        \
          GIDParamBase* _gidParam = _gid_create_enum_param(                   \
            #var_name,                                                        \
            GID_ENUM_PARAM_TYPE_INT64,                                        \
            _gidValues,                                                       \
            sizeof(int64_t),                                                  \
            _gidCount);                                                       \
          _gid_add_param(_gid_added_test, _gidParam);                         \
        }                                                                     \
        else if(_gid_is_this_test_running)                                    \
        {                                                                     \
          _gid_read_variable(int64_t, var_name);                              \
        }

/* Defines a parameter variable that will be tested with multiple predefined
 * unsigned integer values, one at a time.
 * @param var_name - The name that you want to assign to the local uint64_t
 *        variable.
 * @param ... - The set of values with which you want the variable to be
 *        tested.
 * @remarks - This is the same as 'EnumParam', except the local variable
 *          is unsigned rather than signed. */
#define UnsignedEnumParam(var_name, ...)                                      \
        uint64_t var_name;                                                    \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          uint64_t _gidValues[] = {__VA_ARGS__};                              \
          size_t _gidCount = sizeof(_gidValues)/sizeof(_gidValues[0]);        \
          GIDParamBase* _gidParam = _gid_create_enum_param(                   \
            #var_name,                                                        \
            GID_ENUM_PARAM_TYPE_UINT64,                                       \
            _gidValues,                                                       \
            sizeof(uint64_t),                                                 \
            _gidCount);                                                       \
          _gid_add_param(_gid_added_test, _gidParam);                         \
        }                                                                     \
        else if(_gid_is_this_test_running)                                    \
        {                                                                     \
          _gid_read_variable(uint64_t, var_name);                             \
        }

/* Defines a parameter variable that will be tested with multiple predefined
 * null-terminated string values, one at a time.
 * @param var_name - The name that you want to assign to the local char*
 *        variable.
 * @remarks - This is the same as 'EnumParam', except that the local variable
 *          is a char* (string) rather than a signed integer.
 * @example -
 *
 * Test(MyTestFunc,
 *   StringEnumParam(word, "Alpha", "Bravo", "Charlie")
 *   StringEnumParam(noun, "Cat", "Dog")
 * {
 *   //This test will be run 3*2=6 times so each combination of values can
 *   //be tested.
 *
 *   assert_string_not_eq("Bravo", word);
 * }
 *
 * */
#define StringEnumParam(var_name, ...)                                        \
        char* var_name;                                                       \
        if(_gid_is_initializing)                                              \
        {                                                                     \
          char* _gidValues[] = {__VA_ARGS__};                                 \
          size_t _gidCount = sizeof(_gidValues)/sizeof(_gidValues[0]);        \
          GIDParamBase* _gidParam = _gid_create_enum_param(                   \
            #var_name,                                                        \
            GID_ENUM_PARAM_TYPE_STRING,                                       \
            _gidValues,                                                       \
            sizeof(char*),                                                    \
            _gidCount);                                                       \
          _gid_add_param(_gid_added_test, _gidParam);                         \
        }                                                                     \
        else if(_gid_is_this_test_running)                                    \
        {                                                                     \
          _gid_read_variable(char*, var_name);                                \
        }




/* Defines the expected byte at a particular location in a 'corruption
 * detection signature'.
 * @param index - The index of the byte.
 * @returns - The expected byte at the index. */
#define _gid_mem_signature_byte(index) ((uint8_t)((((index)+1))*37))

/* Writes a 'corruption detection signature' to memory.
 * @param dst - The destination buffer.
 * @param offset - The index of the first signature byte to write.
 * @param len - The length of the signature to write. */
void _gid_write_mem_signature(uint8_t* dst, int64_t offset, int64_t len)
{
  for(int64_t i = 0; i < len; i++)
    dst[offset+i] = _gid_mem_signature_byte(offset+i);
}

/* Verifies that a 'corruption detection signature' exists unmodified
 * in a particular portion of memory.
 * @param src - The memory to scan.
 * @param offset - The index of the first byte to scan.
 * @param len - The number of bytes to scan.
 * @returns - Zero if the signature was corrupted, otherwise non-zero. */
int _gid_verify_mem_signature(const uint8_t* src, int64_t offset, int64_t len)
{
  for(int64_t i = 0; i < len; i++)
  {
    if(src[offset+i] != _gid_mem_signature_byte(offset+i))
      return 0;
  }
  return 1;
}

/* Structure that tracks information about memory allocated by gid_malloc. */
typedef struct GIDMallocInfo
{
  /* The size of the payload, as requested by the caller of gid_malloc. */
  int64_t payload_size;

  /* Must be equal to 'payload_size', otherwise this data was corrupted. */
  int64_t payload_size_verify;

  /* Must be equal to 0xDEADBEEF, otherwise this data was corrupted. */
  int32_t deadbeef;

  /* Checksum of the data in this GIDMallocInfo, including any potential padding. */
  int32_t checksum;
} GIDMallocInfo;

/* Calculates the expected 'checksum' field value of a GIDMallocInfo structure.
 * @param info - Pointer to the GIDMallocInfo for which to calculate the checksum.
 * @returns - The calculated checksum. */
int32_t _gid_calc_malloc_info_checksum(const GIDMallocInfo* info)
{
  GIDMallocInfo* inf = (GIDMallocInfo*)info;
  //Backup the stored 'checksum' value
  int32_t checksumBackup = inf->checksum;

  //Write zero to the checksum, since it's not part of calculation
  inf->checksum = 0;

  //Calculate the checksum one byte at a time
  uint8_t* raw = (uint8_t*)info;
  int32_t ret = 0;
  for(size_t i = 0; i < sizeof(GIDMallocInfo); i++)
    ret += (i+1)*11*raw[i];

  //Restore the original checksum so caller's info isn't changed
  inf->checksum = checksumBackup;
  return ret;
}

/* Helper function to allocate memory that, upon free, will detect whether any
 * out-of-bounds writes were performed. This is useful for testing code that
 * initializes data in a buffer, perhaps in a complex manner, to ensure that
 * the initialization code stays within its given bounds. When gid_free is
 * called, the padding to the left and right of the returned memory will
 * be scanned for corruption (and an assert failure will happen if corruption
 * is detected).
 * @param size - The number of bytes to allocate.
 * @returns - A pointer to the allocated memory. */
void* gid_malloc(int64_t size)
{
  if(size < 0)
    return NULL;
  size_t internalSize = GID_MALLOC_PADDING + sizeof(GIDMallocInfo) + size + GID_MALLOC_PADDING;
  uint8_t* mem = malloc(internalSize);
  if(mem == NULL)
    return NULL;

  /* Write a corruption detection signature to the entire buffer.
    This has two benefits: Upon free, it lets us check if the
    'outside' memory was corrupted (hinting that the code wrote memory
    out of bounds, a very bad situation!). It also simulates garbage
    initial data, which helps prove that the application's initialization
    code is correct. */
  _gid_write_mem_signature(mem, 0, internalSize);

  /* Write information about the allocation just before the returned memory portion. */
  GIDMallocInfo* info = (GIDMallocInfo*)(mem+GID_MALLOC_PADDING);
  info->payload_size = size;
  info->payload_size_verify = size;
  info->deadbeef = 0xDEADBEEF;
  int32_t infoChecksum = _gid_calc_malloc_info_checksum(info);
  info->checksum = infoChecksum;

  /* Only give the caller the 'interior' portion of memory. */
  return (void*)(mem+GID_MALLOC_PADDING+sizeof(GIDMallocInfo));
}

/* Checks whether memory was corrupted outside of the allocated bounds,
 * and frees the allocated memory.
 * @param src - Pointer to the memory that was allocated by gid_malloc.
 * @returns - Zero if corruption was detected outside of the bounds that
 *          were originally allocated by gid_malloc, otherwise non-zero. */
int _gid_free_and_check(uint8_t* src)
{
  if(src == NULL)
    return 0;
  uint8_t* mem = src;

  //Read the GIDMallocInfo for this memory block
  GIDMallocInfo* info = (GIDMallocInfo*)(src-sizeof(GIDMallocInfo));

  //Verify that the GIDMallocInfo wasn't corrupted
  if(info->payload_size != info->payload_size_verify
    || info->deadbeef != 0xDEADBEEF
    || info->checksum != _gid_calc_malloc_info_checksum(info))
    return 0;

  int64_t size = info->payload_size;
  uint8_t* raw = mem - (sizeof(GIDMallocInfo)+GID_MALLOC_PADDING);

  //Check that the corruption-detection paddings weren't corrupted
  int ret = _gid_verify_mem_signature(raw, 0, GID_MALLOC_PADDING)
    && _gid_verify_mem_signature(raw, GID_MALLOC_PADDING+sizeof(GIDMallocInfo)+size, GID_MALLOC_PADDING);

  //Free the memory
  free(raw);

  return ret;
}

/* Frees memory that was allocated by gid_malloc, and asserts that none of the
 * memory outside of the allocated region was corrupted.
 * @param memory - Pointer to the memory that was allocated by gid_malloc. */
#define gid_free(memory)                                                      \
{                                                                             \
  void* __gidloc_mem = (memory);                                              \
  assert_not_null(__gidloc_mem);                                              \
  assert_message(                                                             \
    _gid_free_and_check(__gidloc_mem),                                        \
     "gid_free detected memory corruption. Either you modified memory outside"\
     " of the 'requested region', or you are freeing the wrong pointer.");    \
}

#endif/*GIDUNIT_H*/
