

// define HSA options
// #define HSA_DONT_ASSERT
// define HSA implemntation
// #define HSA_DONT_ASSERT
#define HSA_IMPLEMENTATION
// include hsa.h
#include <hsa.h>

#include <chrono>
#include <iostream>

#define DO_MALLOC

const int sample_size = 4096;
const int alloc_size = 32;
const int repeat_samples = 100;
int main( int arg_n, char** arg_s )
{
	long long time_array_custom[repeat_samples] = { 0 };
	long long time_array_malloc[repeat_samples] = { 0 };
	long long time_array_linear[repeat_samples] = { 0 };

	FreeListAllocator test1 = FreeListAllocator( GIBI( 1 ) );
	LinearAllocator lin = LinearAllocator( GIBI( 1 ) );
	char* ptr_array[sample_size];
	std::chrono::steady_clock::time_point begin;
	std::chrono::steady_clock::time_point end;
	std::cout.imbue( std::locale( "" ) );

	for( size_t repeat = 0; repeat < repeat_samples; repeat++ )
	{
#ifdef DO_MALLOC
		begin = std::chrono::high_resolution_clock::now();
#endif // DO_MALLOC
		for( size_t i = 0; i < sample_size; i++ )
		{
			ptr_array[i] = ( char* )lin.Allocate( alloc_size * sizeof( char ) );
			*ptr_array[i] = 0b01111111;
		}
#ifdef DO_MALLOC
		end = std::chrono::high_resolution_clock::now();
		long long linear_alloc_time = std::chrono::duration_cast< std::chrono::nanoseconds >( end - begin ).count();
		time_array_linear[repeat] = linear_alloc_time;
#endif // DO_MALLOC
		lin.Reset();

#ifdef DO_MALLOC
		begin = std::chrono::high_resolution_clock::now();
#endif // DO_MALLOC
		for( size_t i = 0; i < sample_size; i++ )
		{
			ptr_array[i] = ( char* )test1.Allocate( alloc_size * sizeof( char ) );
			*ptr_array[i] = 0b01111111;
		}
#ifdef DO_MALLOC
		end = std::chrono::high_resolution_clock::now();
		long long custom_alloc_time = std::chrono::duration_cast< std::chrono::nanoseconds >( end - begin ).count();
		time_array_custom[repeat] = custom_alloc_time;
#endif // DO_MALLOC

		for( size_t i = 0; i < sample_size; i++ )
		{
			test1.Free( ptr_array[i] );
		}
#ifdef DO_MALLOC

		begin = std::chrono::high_resolution_clock::now();
		for( size_t i = 0; i < sample_size; i++ )
		{
			ptr_array[i] = ( char* )malloc( alloc_size * sizeof( char ) );
			*ptr_array[i] = 0b01111111;
		}
		end = std::chrono::high_resolution_clock::now();
		long long malloc_time = std::chrono::duration_cast< std::chrono::nanoseconds >( end - begin ).count();
		time_array_malloc[repeat] = malloc_time;
		for( size_t i = 0; i < sample_size; i++ )
		{
			free( ptr_array[i] );
		}
		std::cout << "Run " << repeat + 1 << ": " << "custom alloc Time Taken: " << custom_alloc_time << std::endl;
		std::cout << "     " << "linear alloc time taken: " << linear_alloc_time << std::endl;
		std::cout << "     " << "malloc Time Taken: " << malloc_time << std::endl;
#endif // DO_MALLOC

	}
#ifdef DO_MALLOC
	//compute averages;
	long long average = 0;
	for( size_t i = 0; i < repeat_samples; i++ )
	{
		average += time_array_custom[i];
	}
	average /= repeat_samples;
	std::cout << "custom alloc average: " << average << std::endl;

	average = 0;
	for( size_t i = 0; i < repeat_samples; i++ )
	{
		average += time_array_malloc[i];
	}
	average /= repeat_samples;
	std::cout << "malloc alloc average: " << average << std::endl;

	average = 0;
	for( size_t i = 0; i < repeat_samples; i++ )
	{
		average += time_array_linear[i];
	}
	average /= repeat_samples;
	std::cout << "Linear alloc average: " << average << std::endl;

	std::getchar();
#endif // DO_MALLOC




	return 0;
}