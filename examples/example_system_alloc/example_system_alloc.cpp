
#define HSA_NO_MALLOC
#define HSA_IMPLEMENTATION
#include <hsa.h>
#include <malloc.h>
/*
This Example shows how to wrap a system allocator for any platform.
By wrapping a system allocator you can use any other allocator provided with this library.
This example uses the aligned version of malloc.
*/

/*
inherit from the base Allocator class. This is a class with 2 pure virtual functions
*/
class MySystemAllocator : public Allocator
{
public:
	MySystemAllocator()
	{
		
	}
	~MySystemAllocator()
	{
	}

private:
	// Inherited via Allocator
	virtual void * Allocate( size_t arg_size, size_t arg_alignment = 0 ) override
	{
		return _aligned_malloc(arg_size, arg_alignment);
	}

	virtual void Free( void * arg_ptr ) override
	{
		_aligned_free( arg_ptr );
	}
};



int main( int arg_n, char** arg_s )
{
	//first init system allocator.
	MySystemAllocator my_system_allocator;
	//now pass this allocator as an argument. the Freelist allocator will now use the system allocator to allocate its buffer.
	FreeListAllocator( MIBI( 100 ), &my_system_allocator ); // 100 mibi bytes allocated using the system allocator.


	return 0;
}