#define HSA_IMPLEMENTATION
// include hsa.h
#include <hsa.h>

#include <functional>
#include <map>
#include <memory>
#include <vector>

int main( int arg_n, char** arg_s )
{
	//create custom allocator.
	FreeListAllocator free_list_alloc = FreeListAllocator( MIBI( 100 ) );

	// create wrapper.
	// template parameter can be anything. every container will make a copy and create a compatible version
	STLAllocatorWrapper<void> std_alloc = STLAllocatorWrapper<void>( &free_list_alloc );

	// give wrapper to vector.
	// again it does not matter what template paramerter is use for the STLAllocatorWrapper
	// but I recommend using void to avoid unnesserary confusion.
	std::vector<int, STLAllocatorWrapper<void>> custom_allocator_vector = std::vector<int, STLAllocatorWrapper<void>>( std_alloc );

	// use std::vector as normal
	custom_allocator_vector.push_back( 3 );
	custom_allocator_vector.push_back( 5 );
	custom_allocator_vector.push_back( 7 );

	// all STL containers work in similar ways.
	std::map<int, size_t, STLAllocatorWrapper<void>> custom_allocator_map = std::map<int, size_t, STLAllocatorWrapper<void>>( std_alloc );


	// shared pointers can also use a custom allocator. this is done by using std::allocate_shared
	// instead of std::make_shared.
	// it is recommended to use std::allocate_shared instead of using the std::shared_ptr constructor.
	// the constructor required a deallocator which is supplied by std::allocate_shared
	std::shared_ptr<int> dd2_shptr = std::allocate_shared<int>( std_alloc, int() );
	
	// to use a custom allocator with unique pointers you have to do a lot more work.
	// for this to work you first have to give the unique_ptr a Destructor function.
	// the easiest thing you can do is use std::function, #include <functional>
	// the std::function needs a single parameter: Type*
	std::unique_ptr<int, std::function<void( int* )>> dd_uptr = {
		new( free_list_alloc.Allocate( sizeof( int ) ) ) int(),	// by allocating memory with the custom allocator and using placement new to construct the object.
		std::bind(															// using bind to group a lambda and 2 parameters together. this will become the Destructor function.
			[]( int* arg_ptr, STLAllocatorWrapper<void> arg_alloc )
				{
					arg_alloc.deallocate( arg_ptr, 0 );						// use custom allocator to deallocate the memory when destructor is called.
				},
			std::placeholders::_1,											// the first parameter is a placeholder. this is the first parameter and will be supplied when the lambda is called.
			std_alloc														// the second parameter is supplied at the creation of the lambda this is your custom allocator.
			)
		};


	std::getchar();
	return 0;
}