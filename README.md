# GIDUnit
Single header file for a C testing framework that makes it easy to write thorough tests (particularly parameterized tests) with only a few lines of code.
The "GID" in GIDUnit stands for "Get It Done," meaining "Get testing done as soon as possible, without sacrificing coverage,
so we can continue working on actual code."

## Usage
Using GIDUnit is easy, you only need to include one header then write code as shown in the following example.
You can also see more detailed examples in the [examples](examples) directory.

### Example Code
```
#include "gidunit.h"

BEGIN_TEST_SUITE(MyFirstTestSuite)

  SetUp()//Optional
  {
    fixture = malloc(sizeof(int));
    assert_not_null(fixture);
    *(int*)fixture = 1;
  }
  
  TearDown()//Optional
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
    EnumParam(i, 3, 6, 9, 12)
    StringEnumParam(word, "Alpha", "Bravo", "Charlie", "Delta", "Echo"))
  {
    //This test will run 2*4*5=40 times
    assert_int_eq(1, *(int*)fixture);
    assert_int_not_eq(0, int_row[0]);
    assert_string_not_eq("Alpha", word);
    assert_int_not_eq(12, i);
  }

END_TEST_SUITE()

int main()
{
  ADD_TEST_SUITE(MyFirstTestSuite);
  ADD_TEST_SUITE(MySecondTestSuite);
  return gidunit();
}

```

## License
All files in this framework/repository are available under two licenses: The Unlicense or The MIT License, whichever you prefer. See [LICENSE](LICENSE) for the full text.
