#include "../gidunit.h"

BEGIN_TEST_SUITE(SimplestExampleSuite)

  Test(MyExample)
  {
    assert(1);
  }

END_TEST_SUITE()

int main()
{
  ADD_TEST_SUITE(SimplestExampleSuite);
  return gidunit();
}
