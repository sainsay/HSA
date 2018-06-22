#define HSA_IMPLEMENTATION
// include hsa.h
#include <hsa.h>

#include <map>
#include <memory>
#include <vector>


template<class T>
using StdVector = std::vector<T, STLAllocatorWrapper<void>>;

struct DataTest2
{
	int data_ = 0;
	int other_data_ = 0;
};

struct DataTest
{
	int data_ = 0;
	int other_data_ = 0;
	int data1_ = 0;
	int other_data1_ = 0;
	int data2_ = 0;
	int other_data2_ = 0;
};

int main( int arg_n, char** arg_s )
{
	//create custom allocator.
	FreeListAllocator free_list_alloc = FreeListAllocator( MIBI( 100 ) );

	// create wrapper.
	// template parameter can be anything. every container will make a copy and create a compatible version
	STLAllocatorWrapper<void> std_alloc = STLAllocatorWrapper<void>( &free_list_alloc );

	// give wrapper to vector.
	// again it does not matter what template paramerter is use for the STLAllocatorWrapper
	// but i recommend using void to avoid unnesserary confusion.
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
	std::shared_ptr<DataTest2> dd2_shptr = std::allocate_shared<DataTest2>( std_alloc, DataTest2() );


	std::getchar();
	return 0;
}