
#ifndef _UNIT_STATE_PRIVATE_H
#define _UNIT_STATE_PRIVATE_H

#include <algorithm>
#include <stdint.h>
#include <vector>

// Simple compatibility layer for AnsiString that uses a c++ std::string under the covers.
// This allows me to develop using modern c++ tooling while still letting it build in Borland
#ifdef __BORLANDC__
#include <vcl.h>
#else
#include <string>

typedef std::string AnsiString;
// Really terrible way to rename the AnsiString Length to the cpp string length
#define Length length
#define SetLength resize
// wrapper definition for IntToStr to use std::to_string();
AnsiString IntToStr(int val) {
    return std::to_string(val);
}

#endif

// Common typedefs that shorten the fullname of various common int types
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

/**
 * Helper functions that can read/write POD (plain old data) types to a vector.
 * NOTE: this is generally not portable between two different machines, so it should not
 * be used to write data that will be saved in a file. A more robust serialization format
 * should be used if persistence is required.
 */
namespace ValueSerialize {
    struct Headers {
        enum HeaderValues {
            EMPTY = 0,
            FIXED,
            VARIABLE,
        };
    };
    typedef Headers::HeaderValues Header;

    template < typename Base, typename PotentialDerived >
    struct is_base
    {
        typedef char(&no)[1];
        typedef char(&yes)[2];

        static yes check(Base*);
        static no  check(...);

        enum { value = sizeof(check(static_cast<PotentialDerived*>(0))) == sizeof(yes) };
    };

    template <typename T, std::size_t Size = sizeof(T)>
    inline void Write(std::vector<u8>& out, const T& value, const std::size_t length = Size) {
        // Special case for writing a single byte
        if (length == 1) {
            out.push_back((u8)value);
            return;
        }
        const u8* data = reinterpret_cast<const u8*>(&value);
        for (u32 count = 0; count < length; ++count) {
            out.push_back(data[count]);
        }
    }

    template <typename T, std::size_t Size = sizeof(T)>
    inline void Read(const std::vector<u8>& in, T* out, std::size_t& index, const std::size_t length = Size) {
        for (u32 count = 0; count < length; ++count) {
            out[count] = in[index++];
        }
    }

    template <>
    inline void Write(std::vector<u8>& out, const Header& value, const std::size_t _length) {
        out.push_back((u8)value);
    }
    template <>
    inline void Read(const std::vector<u8>& in, Header* out, std::size_t& index, const std::size_t _length) {
        out[0] = (Header)in[index++];
    }

    // Specialization for Reading/Writing variable length arrays
    template <>
    inline void Write(std::vector<u8>& out, const std::vector<u8>& value, const std::size_t length) {
        Write<u8>(out, value[0], length);
    }
    template <>
    inline void Read(const std::vector<u8>& in, std::vector<u8>* out, std::size_t& index, const std::size_t length) {
        Read<u8>(in, &(*out)[0], index, length);
    }


    class Interface {
    public:

        virtual ~Interface() {}
        virtual void CreateDiff(std::vector<u8>& patch, const Interface& other) const {
            CreateDiffInner(patch, other);
        }
        virtual void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) {
            ApplyDiffInner(patch, index);
        }

        virtual inline u8* GetRawData() const = 0;
        virtual inline u32 GetSize() const = 0;
        virtual inline void Resize(u32 newlen) {};

        inline void CreateDiffInner(std::vector<u8>& patch, const Interface& other) const {
            u32 sum = 0;
            u8* thisdata = this->GetRawData();
            u8* otherdata = other.GetRawData();
            u32 thislen = this->GetSize();
            u32 otherlen = other.GetSize();
            u32 patchsize = std::max(thislen, otherlen);

            std::vector<u8> xorpatch;
            for (u32 count = 0; count < patchsize; ++count) {
                u8 val = thisdata[count] ^ otherdata[count];
                sum += val;
                xorpatch.push_back(val);
            }
            if (thislen != otherlen) {
                Write<Header>(patch, Headers::VARIABLE);
                Write<u32>(patch, patchsize);
                Write<u32>(patch, thislen);
                Write<u32>(patch, otherlen);
            }
            else if (sum != 0 && thislen == otherlen) {
                Write<Header>(patch, Headers::FIXED);
                Write<u32>(patch, thislen);
            }
            else {
                Write<Header>(patch, Headers::EMPTY);
                return;
            }
            Write<std::vector<u8> >(patch, xorpatch, xorpatch.size());
        }

        inline void ApplyDiffInner(const std::vector<u8>& patch, std::size_t& index) {
            Header header;
            Read<Header>(patch, &header, index);
            if (header == Headers::EMPTY) {
                return;
            }
            u32 patchsize;
            u32 oldlen;
            u32 newlen;
            if (header == Headers::FIXED) {
                Read<u32>(patch, &patchsize, index);
                oldlen = patchsize;
                newlen = patchsize;
            }
            else {
                Read<u32>(patch, &patchsize, index);
                Read<u32>(patch, &oldlen, index);
                Read<u32>(patch, &newlen, index);
            }

            u32 finallen = patchsize ^ oldlen ^ newlen;
            if (oldlen != newlen) {
                this->Resize(finallen);
            }

            u8* data = this->GetRawData();

            for (u32 count = 0; count < finallen; ++count) {
                data[count] ^= patch[index++];
            }
        }
    };

    template <typename T, u32 Size = sizeof(T)>
    class Fixed : public Interface {
    public:
        Fixed(T* _data) : Interface(), data(_data), hasInnerInterface() {
            // Check if the inner type is also a data container. If it is,
            // use the inner type's GetRawData instead.
            hasInnerInterface = is_base<T, Interface>::value;
        }
        ~Fixed() override {
            // Check if the inner type is also a data container. If it is,
            // then we need to free this inner class as well
            if (hasInnerInterface) {
                delete data;
            }
        }
        inline u32 GetSize() const override { return Size; }
        inline u8* GetRawData() const override {
            if (hasInnerInterface) {
                const Interface* p = reinterpret_cast<const Interface*>(data);
                return p->GetRawData();
            }
            return reinterpret_cast<u8*>(data);
        }
    private:
        T* data;
        bool hasInnerInterface;
    };

    template <typename T>
    class Resizeable : public Interface {
    public:
        Resizeable(T* _data) : Interface(), data(_data) {}
    private:
        T* data;
    };

    template <>
    class Resizeable<std::vector<u8> > : public Interface {
    public:
        Resizeable(std::vector<u8>* _data) : data(_data) {}

        u32 GetSize() const override { return data->size(); }
        u8* GetRawData() const override { return &(*data)[0]; }
        void Resize(u32 newlen) override { data->resize(newlen); }
    private:
        std::vector<u8>* data;
    };

    template<>
    class Resizeable<AnsiString> : public Interface {
    public:
        Resizeable(AnsiString* _data) : data(_data) {}

        //void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) override {
        //    // In older c++ versions, the data in the string is not guaranteed to be contiguous so
        //    // Copy the AnsiString into a vector, then call ApplyDiffInner on the vector
        //    temp.clear();
        //    temp.insert(temp.end(), data->c_str(), data->c_str() + data->Length());
        //    Resizeable<std::vector<u8> > v(&temp);
        //    v.ApplyDiffInner(patch, index);
        //    // and then copy it back into the string
        //    (*data) = AnsiString(reinterpret_cast<const char*>(&temp[0]), temp.size());
        //}

        u32 GetSize() const override { return data->Length(); }
        u8* GetRawData() const override { return reinterpret_cast<u8*>(const_cast<char*>(data->c_str())); }
        void Resize(u32 newlen) override { data->SetLength(newlen); }
    private:
        //std::vector<u8> temp;
        AnsiString* data;
    };
}

template <typename T>
class WeakRef {
public:
    WeakRef() : backing(nullptr) {}
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

    inline void Set(T* newpointer) { backing = newpointer; }

private:
    T* backing;
};

//
///**
//* Abstract base class that represents a value that can be diffed and serialized.
//*/
//class Value {
//public:
//    Value(void* _data) : data(_data) {}
//    virtual void CreateDiff(std::vector<u8>& out, const Value& other) const = 0;
//    virtual void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) = 0;
//    void* data;
//};
//
///**
// * Implements the Value class to support a Single POD value that can be diffed.
// * This class includes a second `global` field to update both the current state field
// * and the global copy of the field whenever its changed.
// * 
// * The structure in the diff file for Single is
// * 1 byte - is empty
// * 4 bytes - Length of the following data
// * 0 or N bytes - Data for the value. If the length is zero, then this field is omitted.
// */
//template <typename T, std::size_t Size = sizeof(T)>
//class Fixed : public Value {
//public:
//    Fixed(T* _data, T* _global = nullptr) : Value(_data), global(_global) {}
//
//    void CreateDiff(std::vector<u8>& out, const Value& other) const override {
//        // Start off by checking the difference between our data and the other's data
//        // by `XOR` ing between the two. This only works for POD types but there isn't a 
//        T diff = (*reinterpret_cast<T*>(this->data)) ^ (*reinterpret_cast<T*>(other.data));
//        ValueSerialize::CreateDiff()
//    }
//
//    void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) override {
//        // If theres no patch, just exit early.
//        if (IsEmpty(patch, index)) {
//            return;
//        }
//        // Read the value from the patch and apply it to our current data
//        T val = reader(patch, index);
//        (*reinterpret_cast<T*>(this->data)) ^= val;
//        // Due to Single types not being pointers, we have to update the global copy as well
//        global ^= val;
//    }
//
//private:
//    T* global;
//};
//
///**
// * Implements the Value class to support a Fixed size List of Value.
// *
// * The structure in the diff file for FixedLen is
// * 1 byte - Set to 0 if there is no changes in the patch, otherwise non zero
// * 0 or N bytes - Data for the value. There is exactly `length` number of bytes
// */
//template <typename T, std::size_t Len>
//class FixedLen : public Value {
//public:
//    FixedLen(T* _data) : Value(_data) {}
//
//    void CreateDiff(std::vector<u8>& out, const Value& other) const override {
//        // Start off by checking the difference between our data and the other's data
//        // by `XOR` ing between the two.
//        T left, right, diff;
//        T* olddata = reinterpret_cast<T*>(other.data);
//        T* newdata = reinterpret_cast<T*>(this->data);
//
//        std::vector<u8> xorpatch;
//        xorpatch.reserve(length);
//
//        // Check to see if anything changed by keeping a running total of all the diffs
//        u32 sum = 0;
//        for (u32 i = 0; i < Len; ++i) {
//            left = olddata[i];
//            right = newdata[i];
//            diff = left ^ right;
//            // Simple way to check if theres any changes. If there is no changes
//            // we always just add zero, otherwise we will add a number.
//            // since its unsigned, this will only increase.
//            sum += diff;
//            // This uses the writer to add variable sized types into the vec<u8>
//            // so it should be safe with variable numeric types
//            WriteVec<T>(xorpatch, diff);
//        }
//        if (sum == 0) {
//            // If the two values are the same, we just push 0 as the patch length to indicate no diff
//            WriteVec<u8>(out, 0);
//            return;
//        }
//        // Otherwise, write nonzero for the "isempty" flag followed by the value
//        WriteVec<u8>(out, 1);
//        out.insert(out.end(), xorpatch.begin(), xorpatch.end());
//    }
//
//    void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) override {
//        u8 isempty = ReadVec<u8>(patch, index);
//        if (isempty == 0) {
//            // If theres no patch, just exit early.
//            return;
//        }
//        T* data = reinterpret_cast<T*>(this->data);
//        // Read the value from the patch and apply it to our current data
//        for (u32 i = 0; i < length; ++i) {
//            T val = ReadVec<T>(patch, index);
//            data[i] ^= val;
//        }
//    }
//
//private:
//    u32 length;
//};
//
//
///**
// * Implements the Value class to support a Variable length vector of Value.
// *
// * The structure in the diff file for VariableLen is
// * 1 byte - Set to 0 if there is no changes in the patch, otherwise non zero
// * If nonzero then the following bytes are added
// * 4 bytes - Length of the old data
// * 4 bytes - Length of the new data
// * N bytes - XOR patch for the value
// */
//template <typename T>
//class VariableLen : public Value {
//public:
//    VariableLen(std::vector<T>* _data) : Value(_data) {}
//
//    void CreateDiff(std::vector<u8>& out, const Value& other) const override {
//        T left, right, diff;
//        const std::vector<T>& olddata = *reinterpret_cast<std::vector<T>*>(other.data);
//        const std::vector<T>& newdata = *reinterpret_cast<std::vector<T>*>(this->data);
//
//        u32 oldlen = olddata.size();
//        u32 newlen = newdata.size();
//        u32 patchsize = std::max(oldlen, newlen);
//        std::vector<u8> xorpatch;
//        xorpatch.reserve(patchsize + 12); // + 12 to reserve extra room for the lengths
//
//        // Check to see if anything changed by keeping a running total of all the diffs
//        u32 sum = 0;
//        for (u32 i = 0; i < patchsize; ++i) {
//            left = (i < oldlen) ? olddata[i] : 0;
//            right = (i < newlen) ? newdata[i] : 0;
//            diff = left ^ right;
//            // Simple way to check if theres any changes. If there is no changes
//            // we always just add zero, otherwise we will add a number.
//            // since its unsigned, this will only increase.
//            sum += diff;
//            // This uses the writer to add variable sized types into the vec<u8>
//            // so it should be safe with variable numeric types
//            WriteVec<T>(xorpatch, diff);
//        }
//        if (sum == 0) {
//            // If the two values are the same, we just push 0 as the patch length to indicate no diff
//            WriteVec<u8>(out, 0);
//            return;
//        }
//        // Otherwise, write nonzero for the "isempty" flag followed by the value
//        WriteVec<u8>(out, 1);
//        WriteVec<u32>(out, oldlen);
//        WriteVec<u32>(out, newlen);
//        out.insert(out.end(), xorpatch.begin(), xorpatch.end());
//    }
//
//    void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) override {
//        T left, right;
//        std::vector<T>& currdata = *reinterpret_cast<std::vector<T>*>(this->data);
//
//        u8 isempty = ReadVec<u8>(patch, index);
//        if (isempty == 0) {
//            return;
//        }
//
//        u32 oldlen = ReadVec<u32>(patch, index);
//        u32 newlen = ReadVec<u32>(patch, index);
//        u32 currlen = currdata.size();
//
//        // Fast way to deterimine the new output length if we are doing a resize. The current length is
//        // guaranteed to equal either newlen or old len, and this will make it cancel out with one of those two.
//        // Then all thats left is the other. This will set outlen to which ever of the two does not equal current
//        u32 outlen = currlen ^ newlen ^ oldlen;
//        currdata.resize(outlen);
//        
//        // XOR the original data with the patch and chose the length of the patch
//        for (u32 i = 0; i < outlen; i++) {
//            left = (i < currlen) ? currdata[i] : 0;
//            right = ReadVec<T>(patch, index);
//            currdata[i] = left ^ right;
//        }
//
//        // After we write the data, we need to skip over the rest of the patch
//        // since it won't be used
//        index += std::max(oldlen, newlen) - outlen;
//    }
//};

#endif // _UNIT_STATE_PRIVATE_H
