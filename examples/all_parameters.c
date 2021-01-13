#include "../gidunit.h"

BEGIN_TEST_SUITE(AllParameters)

  Test(AllParamsCombo,
    IntRow(1, 1, 2, 3)
    IntRow(4, 4, 5, 6)
    IntRow(-1, -1, -1, -1)
    UIntRow(1, 2, 3, 4, 3, 2, 1)
    UIntRow(5, 6, 7, 8, 7, 6, 5)
    UIntRow(9, 10, 11, 12, 11, 10, 9)
    StringRow("Alpha", "Bravo", "Charlie")
    StringRow("alpha", "bravo", "charlie")
    StringRow("ALPHA", "BRAVO", "CHARLIE")
    EnumParam(i, -1, -2, -3)
    UnsignedEnumParam(j, 1, 2, 3)
    StringEnumParam(animal, "Cat", "Dog"))
  {
    assert(int_row[0] < 100);
    assert(uint_row[0] < 100);
    assert(string_row[1][2] == string_row[2][2]);
    assert(i < 0);
    assert(j > 0);
    assert(strlen(animal) == 3);
  }

  Test(VariableColsPerRow,
    IntRow(1)
    IntRow(2, 3)
    IntRow(3, 4, 5)
    IntRow(4, 5, 6, 7))
  {
    switch(int_row[0])
    {
      case 1:
        //Don't rely on this, but if you access columns out of bounds, there is zero padding
        assert_int_eq(0, int_row[1]);
        break;

      case 2:
        assert_int_eq(3, int_row[1]);
        break;

      case 3:
        assert_int_eq(5, int_row[2]);
        break;

      case 4:
        assert_int_eq(7, int_row[3]);
        break;

      default:
        assert_fail("Unexpected value in row[0].");
    }
  }

END_TEST_SUITE()

int main()
{
  ADD_TEST_SUITE(AllParameters);
  return gidunit();
}
