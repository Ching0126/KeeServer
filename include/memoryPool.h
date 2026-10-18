#pragma once

#include<cassert>
#include<cstddef>
#include<mutex>
#include<utility>
#include<new>

namespace memoryPool{

#define MEMORY_POOL_NUM 64//内存池的个数
#define SLOT_BASE_SIZE 8//最小槽宽 8 16 32....512
#define MAX_SLOT_SIZE 512//内存池最大请求

struct Slot{
    Slot* next;//一个地址指针 单项链表
};

class MemoryPool{
public:
    explicit MemoryPool(size_t BlockSize = 4096);
    ~MemoryPool();

    void init(size_t size);//初始化
    void* allocate();
    void deallocate(void* ptr);

private:
    void allocateNewBlock();
    size_t padPointer(char* p,size_t align);

    int BlockSize_;//一次向系统要多大？ 4096默认值
    int SlotSize_;//每个槽宽
    Slot* firstBlock_;//大块链表头
    Slot* curSlot_;//最近的空闲槽
    Slot* freeList_;//返回的空闲槽
    Slot* lastSlot_;//槽快边界
    std::mutex mutexForFreeList_;
    std::mutex mutexForBlock_;
};

class HashBucket{//按照大小选口池
public:
    static void initMemoryPool();
    static MemoryPool& getMemoryPool(int index);

    static void* useMemory(size_t size){
        if(size ==0){
            return nullptr;
        }
        if(size >MAX_SLOT_SIZE){
            return operator new(size);
        }
        return getMemoryPool(static_cast<int>((size+7)/SLOT_BASE_SIZE)-1).allocate();//往上取整
    }

    static void freeMemory(void* ptr,size_t size){
        if(!ptr){
            return;
        }
        if(size >MAX_SLOT_SIZE){
            operator delete(ptr);
            return ;
        }
        getMemoryPool(static_cast<int>((size+7)/SLOT_BASE_SIZE)-1).deallocate(ptr);
    }
};

template<typename T,typename... Args>//...表示有不定个数的参数
T* newElement(Args&&... args){
    T* p = reinterpret_cast<T*>(HashBucket::useMemory(sizeof(T)));//获取空间
    if(p){
        new(p) T(std::forward<Args>(args)...);//把东西放在p 不用再找其他地方了。
    }
    return p;
}

template <typename T>
void deleteElement(T* p){
    if(p){
        p->~T();
        HashBucket::freeMemory(p,sizeof(T));
    }
}
}