#include<memoryPool.h>

#include<iostream>
#include<string>

namespace{
struct Foo{
    int x;
    explicit Foo(int v)
        :x(v){
    }
};
}

int main(){
    memoryPool::HashBucket::initMemoryPool();

    void* p = memoryPool::HashBucket::useMemory(7);
    void* q = memoryPool::HashBucket::useMemory(7);
    if(!p || !q || p==q){
        std::cerr<<"Fail: two allocations should be distinct\n";
        return 1;
    }

    memoryPool::HashBucket::freeMemory(p,7);
    void* r = memoryPool::HashBucket::useMemory(7);
    if(r!=p){
        std::cerr<<"Fail:deallocate should recycle via freelist (LTFO)";
        return 1;
    }
    memoryPool::HashBucket::freeMemory(q,7);
    memoryPool::HashBucket::freeMemory(r,7);

    void* big = memoryPool::HashBucket::useMemory(1024);
    if(!big){
        std::cerr<<"Fail:size>512 should still allocate\n";
        return 1;
    }
    memoryPool::HashBucket::freeMemory(big,1024);
    Foo* f = memoryPool::newElement<Foo>(42);
    if(!f || f->x != 42){
        std::cerr<<"Fail: newElement should construct Foo(42)\n";
        return 1;
    }
    memoryPool::deleteElement(f);

    std::cout<<"OK\n";
    return 0;
}