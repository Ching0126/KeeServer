#include <memoryPool.h>

namespace memoryPool{

MemoryPool::MemoryPool(size_t BlockSize)//构造函数
    :BlockSize_(static_cast<int>(BlockSize))
    ,SlotSize_(0)
    ,firstBlock_(nullptr)
    ,curSlot_(nullptr)
    ,freeList_(nullptr)
    ,lastSlot_(nullptr){
}

MemoryPool::~MemoryPool(){///析构函数
    Slot* cur = firstBlock_;//单链表清空
    while(cur){
        Slot* next = cur->next;
        operator delete(reinterpret_cast<void*>(cur));
        cur = next;
    }
}

void MemoryPool::init(size_t size){
    assert(size>0);
    SlotSize_ = static_cast<int>(size);//槽宽s
    firstBlock_ = nullptr;
    curSlot_ = nullptr;
    freeList_ = nullptr;
    lastSlot_ = nullptr;
}

void* MemoryPool::allocate(){//空闲槽指针往后移，当前槽位被分配
    if(freeList_ != nullptr){
        std::lock_guard<std::mutex> lock(mutexForFreeList_);
        if(freeList_ != nullptr){
            Slot* temp = freeList_;
            freeList_ = freeList_->next;
            return temp;
        }
    }
    //无空闲槽位，加内存
    Slot* temp = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutexForBlock_);
        if(curSlot_ >= lastSlot_){
            allocateNewBlock();
        }
        temp = curSlot_;
        curSlot_ += SlotSize_/static_cast<int>(sizeof(Slot));
    }
    return temp;
}

void MemoryPool::deallocate(void* ptr) {//归还用完的槽
    if (!ptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutexForFreeList_);
    //头插法
    reinterpret_cast<Slot*>(ptr)->next = freeList_;//强制转化类型 下一个为freeList_的值，也就是他指的最先的空闲槽
    freeList_ = reinterpret_cast<Slot*>(ptr);
}

//首地址为块头，其地址不作为第一个操控间地址，真正地址从第二槽开始
void MemoryPool::allocateNewBlock() {
    void* newBlock = operator new(BlockSize_);//刚拿到的大块
    reinterpret_cast<Slot*>(newBlock)->next = firstBlock_;
    firstBlock_ = reinterpret_cast<Slot*>(newBlock);//新块头插法
    //大块切槽
    char* body = reinterpret_cast<char*>(newBlock) + sizeof(Slot*);//第二槽地址，槽区
    const size_t paddingSize = padPointer(body, static_cast<size_t>(SlotSize_));
    curSlot_ = reinterpret_cast<Slot*>(body + paddingSize);
    lastSlot_ = reinterpret_cast<Slot*>(
        reinterpret_cast<size_t>(newBlock) + BlockSize_ - SlotSize_ + 1);
        //newBlock) + BlockSize_最后一个地址
        //最后一个槽的起始地址
}

size_t MemoryPool::padPointer(char* p,size_t align){//希望槽区起点是slotsize倍数,取余
    return (align - reinterpret_cast<size_t>(p)) % align;//还要走多少步对齐？
}

void HashBucket::initMemoryPool(){//初始化槽宽
    for(int i = 0;i<MEMORY_POOL_NUM;++i){
        getMemoryPool(i).init(static_cast<size_t>((i+1) * SLOT_BASE_SIZE));
    }
}

MemoryPool& HashBucket::getMemoryPool(int index){//返回第几号内存池
    static MemoryPool pools[MEMORY_POOL_NUM];
    return pools[index];
}
}