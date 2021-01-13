#include "../gidunit.h"
#include <ctype.h>

BEGIN_TEST_SUITE(AllAsserts)

  Test(AssertFailWithFormat,
    StringEnumParam(str, "Hello", "World", "!")
    EnumParam(i, 0, -1, 2)
    UnsignedEnumParam(j, 0, 1, 2))
  {
    //Force assert failure with a specific message with variable values
    assert_fail_format(
      "Forced failure with str=%s, i=%"PRId64", j=%"PRIu64".\n",
      str,
      i,
      j);
  }

  Test(AssertFailPlainMessage)
  {
    //Force assert failure with a plain message
    assert_fail("I was forced to fail.");
  }

  Test(AssertMessageWithFormat, StringEnumParam(str, "Hello", "World"))
  {
    //Test a condition, print a formatted message upon failure
    assert_message_format(
      toupper(str[3])=='A',
      "Expected the 4th character to be an A, but it was %c.",
      toupper(str[3]));
  }

  Test(AssertPlainMessage)
  {
    //Test a condition, print a plain message upon failure
    assert_message(0 == 1, "Expected nonsense.");
  }

  Test(AssertCondition,
    StringRow("Alpha", "Bravo", "Charlie")
    StringRow("alpha", "bravo", "charlie")
    StringRow("ALPHA", "BRAVO", "CHARLIE"))
  {
    //Test a condition with no specific message
    assert(toupper(string_row[0][0]) == 'Z');
  }

  Test(AssertIntEquals, EnumParam(i, -1, 2, -3, 4))
  {
    //Test that two integers are equal
    assert_int_eq(-1, i);
  }

  Test(AssertIntNotEquals, EnumParam(i, -1, 2, -3, 4))
  {
    //Test that two integers are not equal
    assert_int_not_eq(-1, i);
  }

  Test(AssertUIntEquals, UnsignedEnumParam(i, 0, 1, 2))
  {
    //Test that two unsigned integers are equal
    assert_uint_eq(0, i);
  }

  Test(AssertUIntNotEquals, UnsignedEnumParam(i, 0, 1, 2))
  {
    //Test that two unsigned integers are not equal
    assert_uint_not_eq(0, i);
  }

  Test(AssertPointerEquals, EnumParam(i, 0, 1, 2))
  {
    //Test that two pointers are equal
    int64_t wrongAddr = i;
    int64_t* wrongPtr = &wrongAddr;
    int64_t* enumPtr = &i;
    assert_pointer_eq(wrongPtr, enumPtr);
  }

  Test(AssertPointerNotEquals, EnumParam(i, 0, 1, 2))
  {
    //Test that two pointers are not equal
    int64_t* iPtr = &i;
    int64_t* iPtrClone = &i;
    assert_pointer_not_eq(iPtr, iPtrClone);
  }

  Test(AssertNull)
  {
    //Test that a pointer is NULL
    int i = 0;
    int* ptr = &i;
    assert_null(ptr);
  }

  Test(AssertNotNull)
  {
    //Test that a pointer is not NULL
    int* ptr = NULL;
    assert_not_null(ptr);
  }

  Test(AssertStringEquals, StringEnumParam(word, "Alpha", "Bravo"))
  {
    //Test that two strings have the same value
    assert_string_eq("Bravo", word);
  }

  Test(AssertStringNotEquals, StringEnumParam(word, "Alpha", "Bravo"))
  {
    //Test that two strings do not have the same value
    assert_string_not_eq("Alpha", word);
  }

  Test(AssertDoubleEquals)
  {
    //Test that two double precision floats are equal
    assert_double_eq(0.1, 0.2);
  }

  Test(AssertDoubleNotEquals)
  {
    //Test that two double precision floats are not equal
    assert_double_not_eq(0.1, 0.1);
  }

  Test(AssertMemoryEquals)
  {
    //Allocate two pieces of equal memory
    size_t size = 128;
    char* a = malloc(size);
    char* b = malloc(size);
    for(size_t i = 0; i < size; i++)
      a[i] = b[i] = (char)i;

    //Make one byte different
    b[size/2]++;

    //Test that they are both equal
    assert_memory_eq(a, b, size);
  }

  Test(AsseryMemoryNotEquals)
  {
    //Allocate two pieces of equal memory
    size_t size = 128;
    char* a = malloc(size);
    char* b = malloc(size);
    for(size_t i = 0; i < size; i++)
      a[i] = b[i] = (char)i;

    //Test that they are not equal
    assert_memory_not_eq(a, b, size);
  }

END_TEST_SUITE()

int main()
{
  ADD_TEST_SUITE(AllAsserts);
  return gidunit();
}
