

// define HSA options
// #define HSA_DONT_ASSERT
// #define HSA_NO_MALLOC

// define HSA implemntation
#define HSA_IMPLEMENTATION

// include hsa.h
#include <hsa.h>

int main( int arg_n, char** arg_s )
{
	// Linear allocator. Default pool size is 50 Mibibytes ~53.6 Megabytes
	LinearAllocator lin_alloc = LinearAllocator();

	void* void_ptr = lin_alloc.Allocate( MIBI( 1 ) ); // 1 MIBI allocation.
	lin_alloc.Reset(); //no deallocation possible so resetting will make the linear allocator start from the beginning.

	// Stack allocator. Default pool size is 50 Mibibytes ~53.6 Megabytes
	StackAllocator stack_alloc = StackAllocator();

	void_ptr = stack_alloc.Allocate( MIBI( 1 ) ); // 1 MIBI allocation
	stack_alloc.Free( void_ptr ); // mark block as free

	// Bitmap allocator. template argument is block size. default block count is 512 blocks.
	BitmapAllocator<KIBI( 1 )> bitmap_alloc = BitmapAllocator<KIBI( 1 )>();

	void_ptr = bitmap_alloc.Allocate(); // finds a free block.
	bitmap_alloc.Free( void_ptr ); // sets block to free.

	// Free list allocator. Default pool size is 50 Mibibytes ~53.6 Megabytes
	FreeListAllocator free_list_alloc = FreeListAllocator();

	void_ptr = free_list_alloc.Allocate(MIBI(1)); // 1 MIBI allocation
	free_list_alloc.Free( void_ptr ); // Add memory back into the free list

	return 0;
}