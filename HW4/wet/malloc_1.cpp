#include <unistd.h>

void* smalloc(size_t size)
{
    if(size == 0 || size > 100000000)
        {
                return nullptr;
        }
    void* allocatedPtr = sbrk(0);
    if(allocatedPtr == (void*)-1)
    {
        return nullptr;
    }
    if (sbrk(size) == (void*)-1)
    {
            return nullptr;
    }
    return allocatedPtr;
}
