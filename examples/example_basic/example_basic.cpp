

// define HSA options
// #define HSA_DONT_ASSERT
// define HSA implemntation
#define HSA_IMPLEMENTATION
// include hsa.h
#include <hsa.h>

#include <chrono>
#include <iostream>

int main( int arg_n, char** arg_s )
{
	char* test_ptr = nullptr;
	BitmapAllocator<64> test1 = BitmapAllocator<64>(512); 
	for( size_t total_runs = 0; total_runs < 100; total_runs++ )
	{
		for( size_t repeat = 0; repeat < 1000; repeat++ )
		{
			auto begin = std::chrono::high_resolution_clock::now();
			for( size_t i = 0; i < 512; i++ )
			{
				test_ptr = ( char* )test1.Allocate();
				*test_ptr = 0b01111111;
				test1.Free( test_ptr );
			}
			auto end = std::chrono::high_resolution_clock::now();

			long long custom_alloc_time = std::chrono::duration_cast< std::chrono::nanoseconds >( end - begin ).count();

			begin = std::chrono::high_resolution_clock::now();
			for( size_t i = 0; i < 512; i++ )
			{
				test_ptr = ( char* )malloc( 64 * sizeof( char ) );
				*test_ptr = 0b01111111;
				free( test_ptr );
			}
			end = std::chrono::high_resolution_clock::now();
			long long malloc_time = std::chrono::duration_cast< std::chrono::nanoseconds >( end - begin ).count();

			std::cout << "custom alloc Time Taken:" << custom_alloc_time << std::endl;
			std::cout << "malloc Time Taken:" << malloc_time << std::endl;

		}
	}

	std::getchar();
	return 0;
}