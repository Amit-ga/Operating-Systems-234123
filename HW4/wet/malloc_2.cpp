#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

struct MallocMetadata {
    size_t size;
    bool is_free = false;
    MallocMetadata* next = nullptr;
    MallocMetadata* prev = nullptr;
};

MallocMetadata* _memoryBlocks = nullptr;

void _print_list(){
    int i=0;
    for(MallocMetadata* iter = _memoryBlocks;
        iter != nullptr;
        iter = iter->next){
        std::cout << i << ": Size: " << iter->size << std::endl;
        std::cout << i << ": Free: " << iter->is_free << std::endl;
        std::cout << i << ": Mem: " << iter << std::endl << std::endl;
        i++;
    }
}

MallocMetadata* _findFirstFree(size_t size)
{
    if(_memoryBlocks == nullptr || size <= 0)
    {
        return nullptr;
    }
    MallocMetadata* iter = _memoryBlocks;
    while(iter->next!= nullptr)
    {
        if(iter->is_free && size <= iter->size)
        {
            return iter;
        }
        iter = iter->next;
    }
    //if we arrived here, this is the last block in the list, we will return and handle it
    return iter;
}

void* smalloc(size_t size)
{
    if(size == 0 || size > 100000000)
    {
        return nullptr;
    }
    MallocMetadata* block = _findFirstFree(size);
    //if empty or last and not free
    if(block == nullptr || (block->next == nullptr && !block->is_free))
    {
        void* block_ptr = sbrk(0);
        if(sbrk(sizeof(MallocMetadata) + size) == (void*) - 1)
        {
            return nullptr;
        }
        MallocMetadata* new_block = (MallocMetadata*)block_ptr;
        new_block->size = size;
        new_block->is_free = false;
        //empty
        if(block == nullptr)
        {
            _memoryBlocks = new_block;
        }
        //last
        else
        {
            block->next = new_block;
            new_block->prev = block;
        }
        return (void*)((char*)(new_block) + sizeof(MallocMetadata));
    }
    //some free block in the list, removing else because of return
    block->is_free = false;
    return (void*)((char*)(block) + sizeof(MallocMetadata));
}

void* scalloc(size_t num, size_t size)
{
    int real_size = num * size;
    void* block_ptr = smalloc(real_size);
    if(block_ptr != nullptr)
    {
        memset(block_ptr, 0, real_size);
    }
    return block_ptr;
}

void sfree(void *p)
{
    if(p == nullptr)
    {
        return;
    }
    MallocMetadata* block =(MallocMetadata*)((char*)p - sizeof (MallocMetadata));
    if(block->is_free)
    {
        return;
    }
    block->is_free = true;
}

void* srealloc(void* oldp, size_t size)
{
    if(size == 0 || size > 100000000)
    {
        return nullptr;
    }
    if(oldp == nullptr)
    {
        return smalloc(size);
    }
    MallocMetadata* block =(MallocMetadata*)((char*)oldp - sizeof (MallocMetadata));
    if(size <= block->size){
        return oldp;
    }
    void* block_ptr = smalloc(size);
    if(block_ptr != nullptr)
    {
        memmove(block_ptr, oldp, block->size);
        sfree((void*)((char*)(block) + sizeof (MallocMetadata)));
    }
    return block_ptr;
}

size_t _counting_func(int flag)
{
    size_t size = 0;
    if(_memoryBlocks == nullptr)
    {
        return size;
    }
    
    for(MallocMetadata* iter = _memoryBlocks;
        iter != nullptr;
        iter = iter->next)
    {
        switch (flag){
            case 1:
                if(iter->is_free){
                    size++;
                }
                break;
            case 2:
                if(iter->is_free){
                    size += iter->size;
                }
                break;
            case 3:
                size++;
                break;
            case 4:
                size += iter->size;
                break;
            default:
                break;
        }
    }
    return size;
}

size_t _num_free_blocks() {
    return _counting_func(1);
}

size_t _num_free_bytes(){
    return _counting_func(2);
}

size_t _num_allocated_blocks(){
    return _counting_func(3);
}

size_t _num_allocated_bytes(){
    return _counting_func(4);
}

size_t _num_meta_data_bytes(){
    return sizeof(MallocMetadata) * _num_allocated_blocks();
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}
