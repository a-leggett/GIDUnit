#include "../gidunit.h"

BEGIN_TEST_SUITE(MyMallocTests)

  Test(CanWriteToAllocatedMemory,
    EnumParam(size, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 16, 32, 64, 128))
  {
    uint8_t* mem = gid_malloc(size);

    //Write to the allocated memory
      for(int64_t i = 0; i < size; i++)
        mem[i]++;

    //Free the memory. This will scan for corruption out of bounds, but we stayed in the requested size+bounds.
    gid_free(mem);
  }

  Test(WriteOutOfLeftBoundsFails,
    EnumParam(size, 0, 1, 10, 128)
    EnumParam(offset, 1, 2, 3, 5, 10, 25))
  {
    uint8_t* mem = gid_malloc(size);

    //Corrupt one byte out of bounds
    mem[-offset]++;

    //Free the memory, causing assert to fail when corruption is detected
    gid_free(mem);
  }

  Test(WriteOutOfRightBoundsFails,
    EnumParam(size, 0, 1, 2, 3, 10, 128)
    EnumParam(offset, 1, 2, 3, 5, 10, 25))
  {
    uint8_t* mem = gid_malloc(size);

    //Corrupt one byte out of bounds
    mem[size+offset]++;

    //Free the memory, causing assert to fail when corruption is detected
    gid_free(mem);
  }

END_TEST_SUITE()

int main()
{
  ADD_TEST_SUITE(MyMallocTests);
  return gidunit();
}
