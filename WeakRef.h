
#ifndef _WEAK_REF_H
#define _WEAK_REF_H


// Common typedefs that shorten the fullname of various common int types
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

template <typename T>
class WeakRef {
public:
	WeakRef() : backing(NULL) {}
	WeakRef(T* _backing) : backing(_backing) {}

	WeakRef<T>& operator=(WeakRef<T> other) {
		swap(*this, other);
		return *this;
	}
	WeakRef<T>& operator=(T other) {
		*this->backing = other;
		return *this;
	}

	friend void swap(WeakRef& left, WeakRef& right) {
		std::swap(*left.backing, *right.backing);
	}

	operator T& () { return *backing; }
	T* operator &() { return backing; }

	T operator*() { return *backing; }
	const T operator*() const { return *backing; }

	inline void Set(T* newpointer) { backing = newpointer; }

private:
	T* backing;
};

#endif // _WEAK_REF_H