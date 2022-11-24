
#ifndef _UNIT_STATE_PRIVATE_H
#define _UNIT_STATE_PRIVATE_H

#include <stdint.h>
#include <vector>

// Common typedefs that shorten the fullname of various common int types
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

/**
* Abstract base class that represents a value that can be diffed and serialized.
*/
class Value {
public:
    Value(void* _data) : data(_data) {}
    virtual void CreateDiff(std::vector<u8>& out, const Value& other) const = 0;
    virtual void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) = 0;
    void* data;
};

/**
 * Helper functions that can read/write POD (plain old data) types to a vector.
 * NOTE: this is generally not portable between two different machines, so it should not
 * be used to write data that will be saved in a file. A more robust serialization format
 * should be used if persistence is required.
 */
template <typename T>
inline void WriteVec(std::vector<u8>& out, T value) {
    u32 bytes = sizeof(T);
    u8* data = reinterpret_cast<u8*>(&value);
    for (u32 count = 0; count < bytes; ++count) {
        out.push_back(data[count]);
    }
}

template <typename T>
inline T ReadVec(const std::vector<u8>& in, std::size_t& index) {
    T out = 0;
    u32 bytes = sizeof(T);
    u8* data = reinterpret_cast<u8*>(&out);
    for (u32 count = 0; count < bytes; ++count) {
        data[count] = in[index++];
    }
    return out;
}

// Add a specialization for u8 just because i hate being wasteful
template <>
inline void WriteVec(std::vector<u8>& out, u8 value) {
    out.push_back(value);
}
template <>
inline u8 ReadVec(const std::vector<u8>& out, std::size_t& index) {
    return out[index++];
}
// Add a specialization for Ansistring to treat it as a regular string
template <>
inline void WriteVec(std::vector<u8>& out, AnsiString value) {
    out.push_back(value);
}
template <>
inline AnsiString ReadVec(const std::vector<u8>& out, std::size_t& index) {
    u32 len = ReadVec<u32>(out, index);
    std::vector<const char> tmp;
    tmp.insert(tmp.end(), &out[index], &out[index + len]);

    return AnsiString(tmp.;
}

/**
 * Implements the Value class to support a Single POD value that can be diffed.
 * This class includes a second `global` field to update the both the current state
 * field and the global copy whenever its changed.
 * 
 * The structure in the diff file for Single is
 * 4 bytes - Length of the following data
 * 0 or N bytes - Data for the value. If the length is zero, then this field is omitted.
 */
template <typename T>
class Single : public Value {
public:
    Single(T* _data, T& _global) : Value(_data), global(_global) {}

    void CreateDiff(std::vector<u8>& out, const Value& other) const override {
        // Start off by checking the difference between our data and the other's data
        // by `XOR` ing between the two. This only works for POD types but there isn't a 
        T diff = (*reinterpret_cast<T*>(this->data)) ^ (*reinterpret_cast<T*>(other.data));
        if (diff == 0) {
            // If the two values are the same, we just push 0 as the patch length to indicate no diff
            WriteVec<u32>(out, 0);
            return;
        }
        // Otherwise, write the number of bytes in the type followed by the value
        WriteVec<u32>(out, sizeof(T));
        writer(out, diff);
    }

    void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) override {
        u32 size = ReadVec<u32>(patch, index);
        if (size == 0) {
            // If theres no patch, just exit early.
            return;
        }
        // Read the value from the patch and apply it to our current data
        T val = reader(patch, index);
        (*reinterpret_cast<T*>(this->data)) ^= val;
        // Due to Single types not being pointers, we have to update the global copy as well
        global ^= val;
    }

private:
    T& global;
};

/**
 * Implements the Value class to support a Fixed size List of Value.
 *
 * The structure in the diff file for FixedLen is
 * 1 byte - Set to 0 if there is no changes in the patch, otherwise non zero
 * 0 or N bytes - Data for the value. There is exactly `length` number of bytes
 */
template <typename T>
class FixedLen : public Value {
public:
    FixedLen(T* _data, u32 _length) : Value(_data), length(_length) {}

    void CreateDiff(std::vector<u8>& out, const Value& other) const override {
        // Start off by checking the difference between our data and the other's data
        // by `XOR` ing between the two.
        T left, right, diff;
        T* olddata = reinterpret_cast<T*>(other.data);
        T* newdata = reinterpret_cast<T*>(this->data);

        std::vector<u8> xorpatch;
        xorpatch.reserve(length);

        // Check to see if anything changed by keeping a running total of all the diffs
        u32 sum = 0;
        for (u32 i = 0; i < length; ++i) {
            left = olddata[i];
            right = newdata[i];
            diff = left ^ right;
            // Simple way to check if theres any changes. If there is no changes
            // we always just add zero, otherwise we will add a number.
            // since its unsigned, this will only increase.
            sum += diff;
            // This uses the writer to add variable sized types into the vec<u8>
            // so it should be safe with variable numeric types
            WriteVec<T>(xorpatch, diff);
        }
        if (sum == 0) {
            // If the two values are the same, we just push 0 as the patch length to indicate no diff
            WriteVec<u8>(out, 0);
            return;
        }
        // Otherwise, write nonzero for the "isempty" flag followed by the value
        WriteVec<u8>(out, 1);
        out.insert(out.end(), xorpatch.begin(), xorpatch.end());
    }

    void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) override {
        u8 isempty = ReadVec<u8>(patch, index);
        if (isempty == 0) {
            // If theres no patch, just exit early.
            return;
        }
        T* data = reinterpret_cast<T*>(this->data);
        // Read the value from the patch and apply it to our current data
        for (u32 i = 0; i < length; ++i) {
            T val = ReadVec<T>(patch, index);
            data[i] ^= val;
        }
    }

private:
    u32 length;
};


/**
 * Implements the Value class to support a Variable length vector of Value.
 *
 * The structure in the diff file for VariableLen is
 * 1 byte - Set to 0 if there is no changes in the patch, otherwise non zero
 * If nonzero then the following bytes are added
 * 4 bytes - Length of the old data
 * 4 bytes - Length of the new data
 * N bytes - XOR patch for the value
 */
template <typename T>
class VariableLen : public Value {
public:
    VariableLen(std::vector<T>* _data) : Value(_data) {}

    void CreateDiff(std::vector<u8>& out, const Value& other) const override {
        T left, right, diff;
        const std::vector<T>& olddata = *reinterpret_cast<std::vector<T>*>(other.data);
        const std::vector<T>& newdata = *reinterpret_cast<std::vector<T>*>(this->data);

        u32 oldlen = olddata.size();
        u32 newlen = newdata.size();
        u32 patchsize = std::max(oldlen, newlen);
        std::vector<u8> xorpatch;
        xorpatch.reserve(patchsize + 12); // + 12 to reserve extra room for the lengths

        // Check to see if anything changed by keeping a running total of all the diffs
        u32 sum = 0;
        for (u32 i = 0; i < patchsize; ++i) {
            left = (i < oldlen) ? olddata[i] : 0;
            right = (i < newlen) ? newdata[i] : 0;
            diff = left ^ right;
            // Simple way to check if theres any changes. If there is no changes
            // we always just add zero, otherwise we will add a number.
            // since its unsigned, this will only increase.
            sum += diff;
            // This uses the writer to add variable sized types into the vec<u8>
            // so it should be safe with variable numeric types
            WriteVec<T>(xorpatch, diff);
        }
        if (sum == 0) {
            // If the two values are the same, we just push 0 as the patch length to indicate no diff
            WriteVec<u8>(out, 0);
            return;
        }
        // Otherwise, write nonzero for the "isempty" flag followed by the value
        WriteVec<u8>(out, 1);
        WriteVec<u32>(out, oldlen);
        WriteVec<u32>(out, newlen);
        out.insert(out.end(), xorpatch.begin(), xorpatch.end());
    }

    void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) override {
        T left, right;
        std::vector<T>& currdata = *reinterpret_cast<std::vector<T>*>(this->data);

        u8 isempty = ReadVec<u8>(patch, index);
        if (isempty == 0) {
            return;
        }

        u32 oldlen = ReadVec<u32>(patch, index);
        u32 newlen = ReadVec<u32>(patch, index);
        u32 currlen = currdata.size();

        // Fast way to deterimine the new output length if we are doing a resize. The current length is
        // guaranteed to equal either newlen or old len, and this will make it cancel out with one of those two.
        // Then all thats left is the other. This will set outlen to which ever of the two does not equal current
        u32 outlen = currlen ^ newlen ^ oldlen;
        currdata.resize(outlen);
        
        // XOR the original data with the patch and chose the length of the patch
        for (u32 i = 0; i < outlen; i++) {
            left = (i < currlen) ? currdata[i] : 0;
            right = ReadVec<T>(patch, index);
            currdata[i] = left ^ right;
        }

        // After we write the data, we need to skip over the rest of the patch
        // since it won't be used
        index += std::max(oldlen, newlen) - outlen;
    }
};

#endif // _UNIT_STATE_PRIVATE_H
