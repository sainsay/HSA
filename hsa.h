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

#if _DEBUG // only used for personal testing. not platform independent
#define HSA_DEBUG
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

#ifdef HSA_DONT_ASSERT
#define HSA_ASSERT(arg)
#else
#include <assert.h>
#define HSA_ASSERT( arg ) assert( arg );
#endif // HSA_DONT_ASSERT

#ifndef HSA_NO_MALLOC
#include <cstdlib>
#endif // !HSA_NO_MALLOC

#include <new>

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

#ifndef HSA_NO_MALLOC
/**
* @brief Default Malloc Allocator 
*/
class MallocAllocator : public Allocator
{
public:
	MallocAllocator() = default;
	~MallocAllocator() = default;

	// Inherited via Allocator
	virtual void* Allocate( size_t arg_size, size_t arg_alignment = 0 ) override;
	virtual void Free( void * arg_ptr ) override;
};
#endif // !HSA_NO_MALLOC
#ifndef HSA_NO_MALLOC
/**
* @brief Default Aligned malloc Allocator
*/
class AlignedMallocAllocator : Allocator
{
public:
	AlignedMallocAllocator() = default;
	~AlignedMallocAllocator() = default;

	// Inherited via Allocator
	virtual void* Allocate( size_t arg_size, size_t arg_alignment = 0 ) override;
	virtual void Free( void * arg_ptr ) override;
};
#endif // !HSA_NO_MALLOC

/**
* @brief STL compatible wrapper usable for std containers and smart pointers
*/
template<class C>
class STLAllocatorWrapper
{
public:
	typedef C value_type;
	/**
	* @brief default constructor
	*/
	STLAllocatorWrapper()
	{
		HSA_ASSERT( false ); //cannot construct STLAllocatorWrapper with no Allocator*
	}
	/**
	* @brief constructor
	* @param Allocator* used for allocating memory
	*/
	explicit STLAllocatorWrapper( Allocator* arg_allocator ):
		allocator_(arg_allocator)
	{

	}
	/**
	* @brief Copy constructor
	* @detail copy constructor that accepts any version of STLAllocatorWrapper
	*/
	template<class U>
	STLAllocatorWrapper( const STLAllocatorWrapper<U>& arg_rhs ):
		allocator_(arg_rhs.allocator_)
	{

	}
	/**
	* @brief move constructor
	* @detail move constructor that accepts any version of STLAllocatorWrapper
	*/
	template<class U>
	STLAllocatorWrapper( STLAllocatorWrapper<U>&& arg_rhs ) : 
		allocator_(arg_rhs.allocator_)
	{
		arg_rhs.allocator_ = nullptr;
	}
	/**
	* @brief destructor
	*/
	~STLAllocatorWrapper()
	{
	}
	/**
	* @brief allocates x times the sizeof ( C )
	* @param amount objects
	*/
	C* allocate( size_t arg_count )
	{
		return reinterpret_cast< C* >( allocator_->Allocate( arg_count * sizeof( C ) ) );
	}
	/**
	* @brief deallocates pointer
	* @param pointer
	* @param size of allocation UNUSED
	*/
	void deallocate( C* arg_ptr, size_t arg_size )
	{
		allocator_->Free( arg_ptr );
	}

private:
	template<class U>
	friend class STLAllocatorWrapper; // friend class other instances of the template.

	Allocator* allocator_; // pointer to allocator used for allocation.
};
/**
* @brief equal operator. equal if Allocator* allocator_ is equal
*/
template<class C, class U>
bool operator==( const STLAllocatorWrapper<C>& arg_lhs, const STLAllocatorWrapper<U>& arg_rhs )
{
	return arg_lhs.allocator_ == arg_rhs.allocator_;
}
/**
* @brief not equal operator. same as equal operator but oposite return value
*/
template<class C, class U>
bool operator!=( const STLAllocatorWrapper<C>& arg_lhs, const STLAllocatorWrapper<U>& arg_rhs )
{
	return arg_lhs.allocator_ != arg_rhs.allocator_;
}

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
	namespace FreeList
	{ 
		const size_t minimum_header_size = 8;
	}
	struct FreeListHeader
	{
		FreeListHeader(void* arg_ptr = nullptr, size_t arg_size = 0) :
			header_ptr_(arg_ptr),
			size_(arg_size)
		{

		}
		bool operator<( FreeListHeader& arg_rhs )
		{
			return reinterpret_cast<size_t>(header_ptr_) < reinterpret_cast<size_t>(arg_rhs.header_ptr_);
		}
		bool operator==( FreeListHeader& arg_rhs )
		{
#ifdef HSA_DEBUG
			if( header_ptr_ == arg_rhs.header_ptr_ )
			{
				if( size_ == arg_rhs.size_ )
				{
					return true;
				}
				else
				{
					HSA_ASSERT( false ); // ptr the same but not size.
				}
			}
#else
			if( header_ptr_ == arg_rhs.header_ptr_ && size_ == arg_rhs.size_ )
			{
				return true;
			}
#endif // HSA_DEBUG
			return false;
		}
		bool operator!=( FreeListHeader& arg_rhs )
		{
			return !( *this == arg_rhs );
		}
		void* header_ptr_;
		size_t size_;
	};
	struct FreeListAllocationHeader
	{
		size_t adjustment_;
		size_t size_;
	};
	/**
	* @brief Ascending ordered single link list
	*/
	template<typename DataType >
	class OrderedList
	{
	public:
		struct Node
		{
			Node() :
				data_(DataType()),
				next_(nullptr)
			{

			}
			~Node() = default;
			DataType data_;
			Node* next_;
		};

		class Iterator
		{
		public:
			Iterator()
				: node_( nullptr )
			{
			}

			DataType& operator *()
			{
				return node_->data_;
			}

			bool operator==( const Iterator &a_Rhs ) const
			{
				return node_ == a_Rhs.node_;
			}

			bool operator!=( const Iterator &a_Rhs ) const
			{
				return !( *this == a_Rhs );
			}

			Iterator& operator++()
			{
				node_ = node_->next_;
				return *this;
			}

			Iterator operator+( int a_Step )
			{
				Node *node = node_;
				while( a_Step > 0 )
				{
					if( node->next_ == nullptr)
					{
						return node;
					}
					node = node->next_;
					--a_Step;
				}
				return Iterator( node );
			}

		private:
			Iterator( Node *a_Node )
			{
				node_ = a_Node;
			}

			Node* node_;

			friend class OrderedList;
		};

		OrderedList( Allocator* arg_allocator = nullptr )
		{
			allocator_ = nullptr;

			if( arg_allocator == nullptr )
			{
				has_custom_allocator_ = false;
#ifndef HSA_NO_MALLOC
				void* mem_block = malloc( sizeof( MallocAllocator ) );
				allocator_ = new( mem_block ) MallocAllocator;
#endif // !HSA_NO_MALLOC
			}
			else
			{
				has_custom_allocator_ = true;
				allocator_ = arg_allocator;
			}
			HSA_ASSERT( allocator_ != nullptr );
			head_ = nullptr;
			void* mem_block = allocator_->Allocate( sizeof( Node ) );
			tail_ = new(mem_block) Node();

			size_ = 0;
		}
		~OrderedList()
		{
			Clear();
			if( has_custom_allocator_ == false )
			{
#ifndef HSA_NO_MALLOC
				static_cast< MallocAllocator* >( allocator_ )->~MallocAllocator();
				free( allocator_ );
#endif // !HSA_NO_MALLOC
			}
		}

		size_t Size() const
		{
			return size_;
		}

		bool IsEmpty() const
		{
			return head_ == nullptr;
		}

		Iterator Begin() const
		{
			return Iterator( head_ == nullptr ? tail_ : head_ );
		}

		Iterator End() const
		{
			return Iterator( tail_ );
		}

		Iterator Insert( DataType arg_data )
		{
			void* mem_block = allocator_->Allocate( sizeof( Node ) );
			Node *node = new(mem_block) Node();
			if( head_ == nullptr )
			{
				node->next_ = tail_;
				head_ = node;
			}
			else
			{
				Node* previous = head_;
				while( previous->data_ < arg_data && previous->next_ != tail_ )
				{
					previous = previous->next_;
				}
				node->next_ = previous->next_;
				previous->next_ = node;
			}
			node->data_ = arg_data;
			++size_;

			return Iterator( node );
		}

		Iterator Find( DataType arg_data )
		{
			Node *node = head_;
			while( node->data_ != arg_data && node != tail_ )
			{
				node = node->next_;
			}
			return Iterator( node );
		}

		void Erase( DataType arg_data )
		{
			Node *node = head_;
			Node *previous = nullptr;
			while( node->data_ != arg_data && node->next_ != tail_ )
			{
				previous = node;
				node = node->next_;
			}
			if( node->data_ == arg_data )
			{
				if( previous == nullptr )
				{
					head_ = node->next_ != tail_ ? node->next_ : nullptr;
				}
				else
				{
					previous->next_ = node->next_;
				}
				node->~Node();
				allocator_->Free(node);
				--size_;
			}
		}

		void Clear()
		{
			Node* node = head_;

			if( node != nullptr )
			{
				while( node->next_ != tail_ )
				{
					Node* free_node = node;
					node = node->next_;
					free_node->~Node();
					allocator_->Free( free_node );
				}
			}
			head_ = nullptr;
			size_ = 0;
		}

	private:
		Allocator * allocator_;
		bool has_custom_allocator_;

		Node* head_;
		Node* tail_;
		size_t size_;
	};
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
	void Init();

	using FreeList = detail::OrderedList<detail::FreeListHeader*>;
	FreeList* free_list_;

	Allocator * allocator_ = nullptr;
	bool has_custom_allocator_ = false;
	char* mem_pool_ = nullptr;
	size_t pool_size_ = 0;
};
#endif // !HSA_INCLUDE_HEADER

#ifdef HSA_IMPLEMENTATION
#ifndef HSA_NO_MALLOC
#include <cstdlib>
#endif // HSA_NO_MALLOC

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
#pragma region MallocAllocatorImplementation
#ifndef HSA_NO_MALLOC
void* MallocAllocator::Allocate( size_t arg_size, size_t arg_alignment )
{
	return malloc( arg_size );
}
void MallocAllocator::Free( void* arg_ptr )
{
	free( arg_ptr );
}

void* AlignedMallocAllocator::Allocate( size_t arg_size, size_t arg_alignement )
{
	return _aligned_malloc( arg_size, arg_alignement );
}
void AlignedMallocAllocator::Free( void* arg_ptr )
{
	_aligned_free( arg_ptr );
}
#endif // HSA_NO_MALLOC
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
#ifndef HSA_NO_MALLOC
		free( mem_pool_ );
#endif // !HSA_NO_MALLOC
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
#ifndef HSA_NO_MALLOC
		free( mem_pool_ );
#endif // !HSA_NO_MALLOC
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
#ifndef HSA_NO_MALLOC
		free( mem_pool_ );
		free( bitmap_ );
#endif // !HSA_NO_MALLOC
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
namespace detail
{
	inline bool Merge2ItrBlocks( OrderedList<FreeListHeader*>& arg_list, OrderedList<FreeListHeader*>::Iterator& arg_lhs,  OrderedList<FreeListHeader*>::Iterator& arg_rhs)
	{
		bool result = false;
		if( arg_lhs != arg_list.End() && arg_rhs != arg_list.End() )
		{
			result = reinterpret_cast<void*>(reinterpret_cast<size_t>((*arg_lhs)->header_ptr_) + (*arg_lhs)->size_) == (*arg_rhs)->header_ptr_;
			if( result )
			{
				( *arg_lhs )->size_ += ( *arg_rhs )->size_;
				arg_list.Erase( ( *arg_rhs ) );
			}
		}
		return result;
	}
}
FreeListAllocator::FreeListAllocator() 
{
#ifndef HSA_NO_MALLOC
	mem_pool_ = static_cast< char* >( malloc( pool_size_ = MIBI( 50 ) ) );
#endif
	HSA_ASSERT( mem_pool_ );
	Init();
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
	HSA_ASSERT( mem_pool_ );
	Init();
}
FreeListAllocator::~FreeListAllocator()
{
	free_list_->~OrderedList();
	allocator_->Free( free_list_ );
	if( has_custom_allocator_ == false )
	{
#ifndef HSA_NO_MALLOC
		static_cast< MallocAllocator* >( allocator_ )->~MallocAllocator();
		free( allocator_ );
#endif // !HSA_NO_MALLOC
		allocator_ = nullptr;
	}
	if( allocator_ )
	{
		allocator_->Free( mem_pool_ );
	}
	else
	{
#ifndef HSA_NO_MALLOC
		free( mem_pool_ );
#endif // !HSA_NO_MALLOC

	}
}
inline void FreeListAllocator::Init()
{
	if( allocator_ == nullptr )
	{
		has_custom_allocator_ = false;
#ifndef HSA_NO_MALLOC
		void* mem_block = malloc( sizeof( MallocAllocator ) );
		allocator_ = new( mem_block ) MallocAllocator;
#endif // !HSA_NO_MALLOC
	}
	else
	{
		has_custom_allocator_ = true;
	}
	HSA_ASSERT( allocator_ != nullptr );

	free_list_ = new (allocator_->Allocate(sizeof(FreeList))) FreeList( allocator_ );
	
	free_list_->Insert( new(allocator_->Allocate(sizeof(detail::FreeListHeader))) detail::FreeListHeader(mem_pool_ , pool_size_ ));
}
inline void* FreeListAllocator::Allocate( size_t arg_size, size_t arg_alignment )
{
	detail::FreeListHeader* free_header = nullptr;
	FreeList::Iterator itr = free_list_->Begin();

	while( itr != free_list_->End() )
	{
		auto* header = *itr;
		size_t aligned_offset = detail::calcAlignedOffset( reinterpret_cast< size_t >( header->header_ptr_ ) + sizeof( detail::FreeListAllocationHeader ), arg_alignment );
		size_t total_size = arg_size + sizeof( detail::FreeListAllocationHeader );
		size_t total_aligned_size = total_size + aligned_offset;

		if( header->size_ >= total_aligned_size )
		{
			free_list_->Erase( header );
			char* raw_ptr = reinterpret_cast<char*>( header->header_ptr_ );
			raw_ptr += aligned_offset;
			detail::FreeListAllocationHeader* alloc_header = reinterpret_cast<detail::FreeListAllocationHeader*>( raw_ptr );
			alloc_header->adjustment_ = aligned_offset;
			alloc_header->size_ = arg_size;
			raw_ptr += sizeof( detail::FreeListAllocationHeader );
			if( header->size_ >= total_aligned_size + sizeof( detail::FreeListAllocationHeader ) + detail::FreeList::minimum_header_size )// split header.
			{
				free_list_->Insert( new( allocator_->Allocate( sizeof( detail::FreeListHeader ) ) )detail::FreeListHeader( raw_ptr + arg_size, header->size_ - total_aligned_size ) );

			}
			else if( header->size_ > total_aligned_size )// bigger than but not big enough.
			{
				alloc_header->size_ += header->size_ - total_aligned_size; // add remainder to the size so it does not get lost.
			}
			return raw_ptr;
		}
		++itr;
	}
	HSA_ASSERT( false ); // out of memory
	return nullptr;;
}
inline void FreeListAllocator::Free( void* arg_ptr)
{
	char* raw_ptr = reinterpret_cast<char*>(arg_ptr);
	raw_ptr -= sizeof( detail::FreeListAllocationHeader );
	auto* alloc_header = reinterpret_cast< detail::FreeListAllocationHeader* >( raw_ptr );
	raw_ptr -= alloc_header->adjustment_;
	size_t mem_size = alloc_header->size_ + alloc_header->adjustment_ + sizeof( detail::FreeListAllocationHeader );
	free_list_->Insert( new( allocator_->Allocate( sizeof( detail::FreeListHeader ) ) ) detail::FreeListHeader( raw_ptr, mem_size ) );
}
inline void FreeListAllocator::Reset()
{
	free_list_->Clear();
	free_list_->Insert( new( allocator_->Allocate( sizeof( detail::FreeListHeader ) ) ) detail::FreeListHeader( mem_pool_, pool_size_ ) );
}
inline void FreeListAllocator::Defragment()
{
	auto itr = free_list_->Begin();
	if( itr != free_list_->End() )
	{
		bool is_done = false;
		while( is_done == false )
		{
			auto next = itr + 1;
			if( next != free_list_->End() )
			{
				bool result = detail::Merge2ItrBlocks(*free_list_, itr, next);
				if( result == false )
				{
					itr = next;
				}
			}
			else
			{
				is_done = true;
			}
		}
	}
}
#pragma endregion
#endif // HSA_IMPLEMENTATION
