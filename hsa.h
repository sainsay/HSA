//#define HSA_IMPLEMENTATION //TODO(Jens) remove before sumbitting
/**
*
*
*/
#ifndef HSA_INCLUDE_HEADER
#define HSA_INCLUDE_HEADER

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define HSAENVIRONMENT64
#else
#define HSAENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define HSAENVIRONMENT64
#else
#define HSAENVIRONMENT32
#endif
#endif

#pragma region UsefulMacros
#ifdef HSAENVIRONMENT32
#define GIBI(arg) arg*1024*1024*1024
#define MIBI(arg) arg*1024*1024
#define KIBI(arg) arg*1024
#else
#ifdef HSAENVIRONMENT64 // using long long values. 
#define GIBI(arg) arg*1024ll*1024ll*1024ll
#define MIBI(arg) arg*1024ll*1024ll
#define KIBI(arg) arg*1024ll
#else //not optimal can only go to 4294967295 +- 3.9GiBi of memory
#define GIBI(arg) arg*1024*1024*1024
#define MIBI(arg) arg*1024*1024
#define KIBI(arg) arg*1024
#endif
#endif // HSAENVIRONMENT
#pragma endregion

#include <iostream>

/**
* @brief Abstract class for allocator implementations
*/
class Allocator
{
public:
	/**
	* @brief pure virtual allocate function
	*/
	inline virtual void* Allocate( size_t arg_size, size_t arg_alignment = 0 ) = 0;
	/**
	* @brief pure virtual free fucntion
	* @param void* to memory location
	*/
	inline virtual void Free( void* arg_ptr ) = 0;
};

/**
* @brief Linear Allocator for quick allocation.
* @details Linear Allocator for quick allocation. Dealocation not possible. Allocator can be reset and reused.
*/
class LinearAllocator : public Allocator
{
public:
	/**
	* @brief Default Constuctor, allocates 50 MiBi using malloc to be used by this allocator.
	*/
	LinearAllocator();
	/**
	* @brief Constuctor, allocates "arg_size" bytes memory to be use by this allocator.
	* @param size
	* @param allocator to be used. if nullptr will use malloc. Default = nullptr
	*/
	LinearAllocator( size_t arg_size, Allocator* arg_allocator = nullptr );
	/**
	* @brief Destructor
	*/
	~LinearAllocator();
	/**
	* @brief moves internal ptr forward.
	* @param size
	* @param alignement, default = 0
	* @return void* to reserved memory
	* @throws bad_alloc. out of memory
	*/
	inline virtual void* Allocate( size_t arg_size, size_t alignment = 0 ) override;
	/**
	* @brief Cannot free memory from linear allocator. function provided for compatibility reasons.
	* @param void*
	* @note empty function.
	*/
	inline virtual void Free( void* arg_ptr ) override;
	/**
	* @brief resets the linear allocator and starts from the beginning again.
	* @attention Previous memory allocations might still be valid. use with care.
	*/
	inline virtual void Reset();
protected:
	Allocator * allocator_ = nullptr;
	char* mem_pool_ = nullptr;
	size_t pool_size_ = 0;
	size_t current_offset_ = 0;
};

namespace detail
{
	struct StackHeader
	{
		StackHeader* previous_header_;
		bool is_free_;
	};
}
/**
* @brief Stack Allocator for quick allocation.
* @details Linear Allocator for quick allocation. Dealocation Possible. behaves like a Stack LIFO.
* if other memory is deallocated the allocator will wait until the newest element is deallocated before reusing this memory.
* it will go back as until it reaches a non deallocated part of memory.
* Allocator can be reset and reused.
*/
class StackAllocator : public Allocator
{
public:
	/**
	@brief Default Constuctor, allocates 50 MiBi using malloc to be used by this allocator.
	*/
	StackAllocator();
	/**
	* @brief Constuctor, allocates "arg_size" bytes memory to be use by this allocator.
	* @param size
	* @param allocator to be used. if nullptr will use malloc. Default = nullptr
	*/
	StackAllocator( size_t arg_size, Allocator* arg_allocator = nullptr );
	/**
	* @brief Destructor
	*/
	~StackAllocator();
	/**
	* @brief moves internal ptr forward.
	* @param size
	* @param alignement, default = 0
	* @return void* to reserved memory
	* @throws bad_alloc. out of memory
	*/
	inline virtual void* Allocate( size_t arg_size, size_t alignment = 0 ) override;
	/**
	* @brief list elements in the stack allocator as free. if last element in stack is freed it will go back as far as possible.
	* @param void*
	*/
	inline virtual void Free( void* arg_ptr ) override;
	/**
	* @brief resets the stack allocator and starts from the beginning again.
	* @attention Previous memory allocations might still be valid. use with care.
	*/
	inline virtual void Reset();
protected:

	Allocator * allocator_ = nullptr;
	char* mem_pool_ = nullptr;
	size_t pool_size_ = 0;
	size_t current_offset_ = 0;
	detail::StackHeader* last_allocated_header = nullptr;
};

namespace detail
{
	template< int I>
	struct bitmapChunk
	{
		char memory[I];
	};
}
/**
* @brief Bitmap Allocator for quick same size allocation.
* @details Bitmap Allocator for quick same size allocation. Dealocation Possible.
* Allocator can be reset and reused.
*/
template <size_t ChunkSize>
class BitmapAllocator
{
public:
	/**
	* @brief Default Constuctor, allocates 512 items of ChunkSize using malloc to be used by this allocator. Bitmap is created with malloc.
	*/
	BitmapAllocator();
	/**
	* @brief Allocates chunk_count items of ChunkSize bytes.
	* @param chunk count
	* @param allocator to be used. if nullptr will use malloc. Default = nullptr
	*/
	BitmapAllocator( size_t arg_chunk_count, Allocator* arg_allocator = nullptr );
	/**
	* @brief Destructor
	*/
	~BitmapAllocator();
	/**
	* @brief Allocates one chunk size of memory
	*/
	inline void* Allocate();
	/**
	* @brief Deallocates one chunk
	* @param chunk pointer
	*/
	inline void Free( void* arg_ptr );
	/**
	* @brief Resets the bitmap allocator
	* @attention Previous memory allocations might still be valid. use with care.
	*/
	inline void Reset();
private:
	Allocator * allocator_ = nullptr;
	size_t chunk_size_ = 0;
	size_t chunk_count_ = 0;
	detail::bitmapChunk<ChunkSize>* mem_pool_ = nullptr;
	unsigned char* bitmap_ = nullptr;
	size_t last_allocate_chunk = -1; // 0 is a valid chunk
};
namespace detail
{
	struct FreeListHeader
	{
		FreeListHeader* next_header_;
		size_t size_;
	};
	namespace freelist
	{
		const size_t kminimum_chunk_size = 64;
	}
}

/**
* @brief Freelist Allocator for general purpose allocation.
* @details Freelist Allocator for general purpose allocation. Dealocation Possible.
* Allocator can be reset and reused.
*/
class FreeListAllocator : public Allocator
{
public:
	/**
	* @brief Default Constuctor, allocates 50 MiBi using malloc to be used by this allocator.
	*/
	FreeListAllocator();
	/**
	* @brief Constuctor, allocates "arg_size" bytes memory to be use by this allocator.
	* @param size
	* @param allocator to be used. if nullptr will use malloc. Default = nullptr
	*/
	FreeListAllocator( size_t arg_size, Allocator* arg_allocator = nullptr );
	/**
	* @brief Destructor
	*/
	~FreeListAllocator();
	/**
	* @brief Allocates requested size of memory in free list
	* @param size
	* @param alignment
	* @return pointer to memory
	*/
	inline virtual void* Allocate( size_t arg_size, size_t alignment = 0 ) override;
	/**
	* @brief marks allocated memory as free in the free list.
	* @param pointer to start of memory.
	*/
	inline virtual void Free( void* ) override;
	/**
	* @brief Resets the free list allocator
	* @attention Previous memory allocations might still be valid. use with care.
	*/
	inline virtual void Reset();
	/**
	* @brief Merges empty chunks of memory
	* @details Merges empty chunks of memory. does not move any data around. all memory allocated is still valid.
	*/
	inline virtual void Defragment();

private:
	Allocator * allocator_ = nullptr;
	char* mem_pool_ = nullptr;
	size_t pool_size_ = 0;

};
#endif // !HSA_INCLUDE_HEADER

#ifdef HSA_IMPLEMENTATION
#include <cstdlib>

#ifdef HSA_DONT_ASSERT
#define HSA_ASSERT(arg)
#else
#include <assert.h>
#define HSA_ASSERT( arg ) assert( arg );
#endif // HSA_DONT_ASSERT

#pragma region HelperFunctions
namespace detail
{
	inline size_t calcAlignedOffset( size_t arg_to_align, size_t arg_alignment )
	{
		if( arg_alignment == 0 )
		{
			return 0;
		}
		size_t aligned_offset = arg_alignment - ( arg_to_align % arg_alignment );
		return aligned_offset == arg_alignment ? 0 : aligned_offset;
	}
}
#pragma endregion
#pragma region LinearAllocatorImplementation
LinearAllocator::LinearAllocator()
{
#ifndef HSA_NO_MALLOC
	mem_pool_ = static_cast< char* >( malloc( pool_size_ = MIBI( 50 ) ) );
#endif // !HSA_NO_MALLOC

		HSA_ASSERT( mem_pool_ )
}
LinearAllocator::LinearAllocator( size_t arg_size, Allocator* arg_allocator ) :
	allocator_( arg_allocator )
{
	if( arg_allocator )
	{
		mem_pool_ = static_cast< char* >( arg_allocator->Allocate( pool_size_ = arg_size, 16 ) );
	}
	else
	{
#ifndef HSA_NO_MALLOC
		mem_pool_ = static_cast< char* >( malloc( pool_size_ = arg_size ) );
#endif
	}
	HSA_ASSERT( mem_pool_ )
}
LinearAllocator::~LinearAllocator()
{
	if( allocator_ )
	{
		allocator_->Free( mem_pool_ );
	}
	else
	{
		free( mem_pool_ );
	}
}
inline void* LinearAllocator::Allocate( size_t arg_size, size_t arg_alignment )
{
	void* ret_ptr = nullptr;
	size_t aligned_offset = detail::calcAlignedOffset( current_offset_, arg_alignment );

	if( current_offset_ + aligned_offset + arg_size <= pool_size_ )
	{
		current_offset_ += aligned_offset;
		ret_ptr = mem_pool_ + current_offset_;
		current_offset_ += arg_size;
	}
	else
	{
		HSA_ASSERT( false ) // out of memory
	}
	return ret_ptr;
}
inline void LinearAllocator::Free( void* arg_ptr )
{
	HSA_ASSERT(false) //you cannot free memory with a linear allocator
}
inline void LinearAllocator::Reset()
{
	current_offset_ = 0;
}
#pragma endregion
#pragma region StackAllocatorImplementation
StackAllocator::StackAllocator()
{
#ifndef HSA_NO_MALLOC
	mem_pool_ = static_cast< char* >( malloc( pool_size_ = MIBI( 50 ) ) );
#endif
	HSA_ASSERT( mem_pool_ )

}
StackAllocator::StackAllocator( size_t arg_size, Allocator* arg_allocator ) :
	allocator_( arg_allocator )
{
	if( arg_allocator )
	{
		mem_pool_ = static_cast< char* >( arg_allocator->Allocate( pool_size_ = arg_size, 16 ) );
	}
	else
	{
#ifndef HSA_NO_MALLOC
		mem_pool_ = static_cast< char* >( malloc( pool_size_ = arg_size ) );
#endif
	}
	HSA_ASSERT( mem_pool_ )
}
StackAllocator::~StackAllocator()
{
	if( allocator_ )
	{
		allocator_->Free( mem_pool_ );
	}
	else
	{
		free( mem_pool_ );
	}
}

inline void* StackAllocator::Allocate( size_t arg_size, size_t arg_alignment )
{
	void* return_ptr = nullptr;
	size_t aligned_offset = detail::calcAlignedOffset( current_offset_ + sizeof(detail::StackHeader), arg_alignment );

	if( current_offset_ + aligned_offset + sizeof( detail::StackHeader ) + arg_size <= pool_size_ )
	{
		detail::StackHeader* header_ptr = reinterpret_cast< detail::StackHeader* >( mem_pool_ + current_offset_  + aligned_offset);
		header_ptr->is_free_ = false;
		header_ptr->previous_header_ = last_allocated_header;
		last_allocated_header = header_ptr;
		return_ptr = mem_pool_ + current_offset_ + aligned_offset + sizeof( detail::StackHeader );
		current_offset_ += arg_size + aligned_offset + sizeof( detail::StackHeader );
	}
	else
	{
		HSA_ASSERT( false ) // out of memory
	}

	return return_ptr;
}
inline void StackAllocator::Free( void* arg_ptr )
{
	if( last_allocated_header == nullptr )
	{
		HSA_ASSERT( false ) //deallocation but never allocated
	}
	if( arg_ptr < mem_pool_ + sizeof( detail::StackHeader ) && arg_ptr > mem_pool_ + pool_size_ )
	{
		HSA_ASSERT( false ) //Deallocating outside of Allocator memory
	}

	char* arg_char_ptr = reinterpret_cast< char* >( arg_ptr );
	detail::StackHeader* header_ptr = reinterpret_cast< detail::StackHeader* >( arg_char_ptr - sizeof( header_ptr ) );
	header_ptr->is_free_ = true;
	if( header_ptr == last_allocated_header )
	{
		detail::StackHeader* temp_header_ptr = header_ptr;
		bool continue_looping = true;
		do
		{
			if( temp_header_ptr->previous_header_ != nullptr )
			{
				if( temp_header_ptr->previous_header_->is_free_ )
				{
					temp_header_ptr = temp_header_ptr->previous_header_;
				}
				else
				{
					continue_looping = false;
					char* temp_ptr = reinterpret_cast< char* >( temp_header_ptr );
					current_offset_ = temp_ptr - mem_pool_;
					last_allocated_header = temp_header_ptr->previous_header_;
				}
			}
			else
			{
				continue_looping = false;
				current_offset_ = 0;
				last_allocated_header = nullptr;
			}
		} while( continue_looping );
	}
}
inline void StackAllocator::Reset()
{
	current_offset_ = 0;
	last_allocated_header = nullptr;
}
#pragma endregion
#pragma region BitmapAllocatorImplementation
template <size_t ChunkSize>
BitmapAllocator<ChunkSize>::BitmapAllocator()
{
#ifndef HSA_NO_MALLOC
	bitmap_ = static_cast< unsigned char* >( malloc( 64 * sizeof( char ) ) ); //allocate bitmap
	mem_pool_ = static_cast< detail::bitmapChunk<ChunkSize>* >( malloc( 512 * ChunkSize ) );
#endif
	HSA_ASSERT( mem_pool_ )
	chunk_size_ = ChunkSize;
	chunk_count_ = 512;
	for( size_t i = 0; i < 64; i++ )
	{
		bitmap_[i] = 0;
	}
}
template <size_t ChunkSize>
BitmapAllocator<ChunkSize>::BitmapAllocator( size_t arg_chunk_count, Allocator* arg_allocator ) :
	allocator_( arg_allocator )
{
	size_t corrected_count = arg_chunk_count + detail::calcAlignedOffset( arg_chunk_count, 8 );
	chunk_size_ = ChunkSize;
	chunk_count_ = corrected_count;
	if( arg_allocator )
	{
		bitmap_ = static_cast< unsigned char* >( arg_allocator->Allocate( ( corrected_count / 8 ) * sizeof( char ) ) );
		mem_pool_ = static_cast< detail::bitmapChunk<ChunkSize>* >( arg_allocator->Allocate( corrected_count * sizeof( detail::bitmapChunk<ChunkSize> ) ) );
	}
	else
	{
#ifndef HSA_NO_MALLOC
		bitmap_ = static_cast< unsigned char* >( malloc( ( corrected_count / 8 ) * sizeof( char ) ) );
		mem_pool_ = static_cast< detail::bitmapChunk<ChunkSize>* >( malloc( corrected_count * sizeof( detail::bitmapChunk<ChunkSize> ) ) );
#endif

	}
	HSA_ASSERT( mem_pool_ )
	for( size_t i = 0, size_t = corrected_count / 8; i < 64; i++ )
	{
		bitmap_[i] = 0b00000000;
	}
}
template <size_t ChunkSize>
BitmapAllocator<ChunkSize>::~BitmapAllocator()
{
	if( allocator_ )
	{
		allocator_->Free( mem_pool_ );
		allocator_->Free( bitmap_ );
	}
	else
	{
		free( mem_pool_ );
		free( bitmap_ );
	}
}
template <size_t ChunkSize>
inline void* BitmapAllocator<ChunkSize>::Allocate()
{
	void* return_ptr = nullptr;
	size_t chunk_to_check = last_allocate_chunk + 1;
	size_t startng_chunk = chunk_to_check;
	if( chunk_to_check >= chunk_count_ )
	{
		chunk_to_check = 0;
	}
	bool chunk_allocated = false;
	do
	{
		size_t bit_pos = ( chunk_to_check ) % 8;
		size_t byte_pos = ( chunk_to_check ) / 8;

		unsigned char bit_result = ( ( bitmap_[byte_pos] >> bit_pos ) & 0b00000001 );
		if( !bit_result )
		{
			return_ptr = mem_pool_ + chunk_to_check; // getting allocation address
			bitmap_[byte_pos] |= 0b00000001 << bit_pos;  // setting bit to 1
			last_allocate_chunk = chunk_to_check;
			chunk_allocated = true;
		}
		else
		{
			chunk_to_check++;
			if( chunk_to_check >= chunk_count_ )
			{
				chunk_to_check = 0;
			}
			if( startng_chunk == chunk_to_check )
			{
				HSA_ASSERT( false )
			}
		}
	} while( !chunk_allocated );

	return return_ptr;
}
template <size_t ChunkSize>
inline void BitmapAllocator<ChunkSize>::Free( void* arg_ptr )
{
	detail::bitmapChunk<ChunkSize>* bm_ptr = reinterpret_cast< detail::bitmapChunk<ChunkSize>* >( arg_ptr );
	size_t index = bm_ptr - mem_pool_;
	size_t bit_pos = ( index ) % 8;
	size_t byte_pos = ( index ) / 8;
	bitmap_[byte_pos] &= ~( 0b00000001 << bit_pos );
}
template <size_t ChunkSize>
inline void BitmapAllocator<ChunkSize>::Reset()
{
	for( size_t i = 0; i < chunk_count_ / 8; i++ )
	{
		bitmap_[i] = 0b00000000;
	}
	last_allocate_chunk = -1;
}
#pragma endregion
#pragma region FreeListAllocatorImplementation
FreeListAllocator::FreeListAllocator() 
{
#ifndef HSA_NO_MALLOC
	mem_pool_ = static_cast< char* >( malloc( pool_size_ = MIBI( 50 ) ) );
#endif
	HSA_ASSERT( mem_pool_ )

}
FreeListAllocator::FreeListAllocator( size_t arg_size, Allocator* arg_allocator ) :
	allocator_( arg_allocator )
{
	pool_size_ = arg_size;
	if( arg_allocator )
	{
		mem_pool_ = static_cast< char* >( arg_allocator->Allocate( pool_size_, 16 ) );
	}
	else
	{
#ifndef HSA_NO_MALLOC
		mem_pool_ = static_cast< char* >( malloc( pool_size_ ) );
#endif
	}
	HSA_ASSERT( mem_pool_ )

}
FreeListAllocator::~FreeListAllocator()
{
	if( allocator_ )
	{
		allocator_->Free( mem_pool_ );
	}
	else
	{
		free( mem_pool_ );
	}
}

inline void* FreeListAllocator::Allocate( size_t arg_size, size_t arg_alignment )
{
	void* return_ptr = nullptr;
	
	return return_ptr;
}
inline void FreeListAllocator::Free( void* arg_ptr)
{

}
inline void FreeListAllocator::Reset()
{
	
}

inline void FreeListAllocator::Defragment()
{
	
}
#pragma endregion
#endif // HSA_IMPLEMENTATION
