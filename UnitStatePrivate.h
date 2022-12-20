
#ifndef _UNIT_STATE_PRIVATE_H
#define _UNIT_STATE_PRIVATE_H

#include <algorithm>
#include <ostream>
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

    /**
     * Basic helper struct and method to easily convert an unsigned char to a hex value when writing to ostream
     */
    struct HexCharStruct {
        unsigned char c;
        HexCharStruct(unsigned char _c) : c(_c) { }
    };

    inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs) {
        return (o << std::hex << static_cast<int>(hs.c));
    }

    inline HexCharStruct hex(unsigned char _c) {
        return HexCharStruct(_c);
    }

    struct Headers {
        enum HeaderValues {
            EMPTY = 0,
            FIXED,
            VARIABLE,
        };
    };
    typedef Headers::HeaderValues Header;

	template <typename T>
	void Write(std::vector<u8>& out, const T& value, const std::size_t length = sizeof(T)) {
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

	template <typename T>
	void Read(const std::vector<u8>& in, T* out, std::size_t& index, const std::size_t length = sizeof(T)) {
        u8* view = reinterpret_cast<u8*>(out);
        for (u32 count = 0; count < length; ++count) {
            view[count] = in[index++];
        }
    }

    template <>
	void Write(std::vector<u8>& out, const Header& value, const std::size_t _length) {
        out.push_back((u8)value);
    }
    template <>
	void Read(const std::vector<u8>& in, Header* out, std::size_t& index, const std::size_t _length) {
        out[0] = (Header)in[index++];
    }

    // Specialization for Reading/Writing variable length arrays
    template <>
	void Write(std::vector<u8>& out, const std::vector<u8>& value, const std::size_t length) {
        Write<u8>(out, value[0], length);
    }
    template <>
    void Read(const std::vector<u8>& in, std::vector<u8>* out, std::size_t& index, const std::size_t length) {
        Read<u8>(in, &(*out)[0], index, length);
    }


    class Interface {
    public:
        Interface(AnsiString _name) : name(_name) {}
        virtual ~Interface() {}
        virtual void CreateDiff(std::vector<u8>& patch, const Interface* other) const {
            u32 sum = 0;
            u8* thisdata = this->GetRawData();
            u8* otherdata = other->GetRawData();
            u32 thislen = this->GetSize();
            u32 otherlen = other->GetSize();
            u32 patchsize = std::max(thislen, otherlen);

            std::vector<u8> xorpatch;
            for (u32 count = 0; count < patchsize; ++count) {
                u8 left = (count < thislen) ? thisdata[count] : 0;
                u8 right = (count < otherlen) ? otherdata[count] : 0;
                u8 val = left ^ right;
                sum += val;
                xorpatch.push_back(val);
            }
            if (sum == 0) {
                Write<Header>(patch, Headers::EMPTY);
                return;
            } else if (thislen != otherlen) {
                Write<Header>(patch, Headers::VARIABLE);
                Write<u32>(patch, patchsize);
                Write<u32>(patch, thislen);
                Write<u32>(patch, otherlen);
            } else {
                Write<Header>(patch, Headers::FIXED);
                Write<u32>(patch, thislen);
            }
            Write<std::vector<u8> >(patch, xorpatch, xorpatch.size());
        }
        virtual void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) {
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
			} else {
                Read<u32>(patch, &patchsize, index);
                Read<u32>(patch, &oldlen, index);
				Read<u32>(patch, &newlen, index);
            }

            u32 finallen = this->GetSize() ^ oldlen ^ newlen;
            if (oldlen != newlen) {
                this->Resize(finallen);
            }

            u8* data = this->GetRawData();

            for (u32 count = 0; count < finallen; ++count) {
                data[count] ^= patch[index++];
            }
            index += patchsize - finallen;
        }

        /**
         * Debugging helper, given a patch, write the indicies and values have changed
         */
        virtual void InspectPatch(const std::vector<u8>& patch, std::size_t& index, std::ostream& out) const {
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

            u32 finallen = this->GetSize() ^ oldlen ^ newlen;
            if (oldlen != newlen) {
                out << "  " << name << " length changed: Oldlen=" << oldlen << " Newlen=" << newlen << " Finallen=" << finallen << std::endl;
            }

            u8* data = this->GetRawData();

            for (u32 count = 0; count < finallen; ++count) {
                u8 val = data[count] ^ patch[index++];
                if (val != data[count]) {
                    out << "  " << name << "[" << count << "]" << " changed: value=" << hex(data[count]) << " patched=" << hex(val) << std::endl;
                }
            }
            index += patchsize - finallen;
        }

		virtual inline u8* GetRawData() const = 0;
		virtual inline u32 GetSize() const = 0;
        virtual inline void Resize(u32 newlen) {};

        AnsiString name;
    };

    template <typename T>
    class Resizeable : public Interface {
    public:
		Resizeable(const AnsiString& name, T* _data) : Interface(name), data(_data) {}
    private:
        T* data;
    };

    template <>
    class Resizeable<std::vector<u8> > : public Interface {
    public:
        Resizeable(const AnsiString& name, std::vector<u8>* _data) : Interface(name), data(_data) {}

		inline u32 GetSize() const { return data->size(); }
		inline u8* GetRawData() const { return &(*data)[0]; }
        inline void Resize(u32 newlen) { data->resize(newlen); }
    private:
        std::vector<u8>* data;
    };

    template<>
    class Resizeable<AnsiString> : public Interface {
    public:
		Resizeable(AnsiString name, AnsiString* _data) : Interface(name), data(_data) {}

		inline u32 GetSize() const { return data->Length(); }
		inline u8* GetRawData() const { return reinterpret_cast<u8*>(const_cast<char*>(data->c_str())); }
		inline void Resize(u32 newlen) { data->SetLength(newlen); }
    private:
        AnsiString* data;
    };

    template <typename T, u32 Size = sizeof(T)>
    class Fixed : public Interface {
    public:
        Fixed(const AnsiString& name, T* _data) : Interface(name), data(_data) {}

		inline u32 GetSize() const { return Size; }
        inline AnsiString* GetData() const { return data; }
		inline u8* GetRawData() const { return reinterpret_cast<u8*>(data); }
    private:
        T* data;
    };

    /**
     * Template specialization to support a fixed list of Resizable AnsiString
     */
    template<u32 Size>
    class Fixed<AnsiString, Size> : public Interface {
    public:
		Fixed(const AnsiString& name, AnsiString* _data) : Interface(name), data(_data) {}

		void CreateDiff(std::vector<u8>& patch, const Interface* other) const {
            // The inner type is actually a "resizable" type, an ansistring, so treat it as such when writing to disk.
            for (u32 count = 0; count < Size; ++count) {
                // This is really nasty, but it could be fixed with some changes.
                // We just need to get the inner list of AnsiString so we can iterate over them, but we want
                // to serialize them as Resizeable, so thats why we create a Resizeable out of them
                const Fixed<AnsiString, Size>* casted = dynamic_cast<const Fixed<AnsiString, Size>* >(other);
                AnsiString* raw = casted->GetData();
                Resizeable<AnsiString> left(name + IntToStr(count), & data[count]);
                const Resizeable<AnsiString> right(name + IntToStr(count), &raw[count]);
                left.CreateDiff(patch, &right);
            }
        }
        void ApplyDiff(const std::vector<u8>& patch, std::size_t& index) {
            // The inner type is a resizable, so treat it as such
            for (u32 count = 0; count < Size; ++count) {
                Resizeable<AnsiString> self(name + IntToStr(count), &data[count]);
                self.ApplyDiff(patch, index);
            }
        }
        void InspectPatch(const std::vector<u8>& patch, std::size_t& index, std::ostream& out) {
            // The inner type is a resizable, so treat it as such
            for (u32 count = 0; count < Size; ++count) {
                Resizeable<AnsiString> self(&data[count]);
                self.InspectPatch(patch, index, out);
            }
        }
        inline u32 GetSize() const { return Size; }
        inline AnsiString* GetData() const { return data; }
		inline u8* GetRawData() const { return reinterpret_cast<u8*>(data); }
    private:
        AnsiString* data;
    };
}
#endif // _UNIT_STATE_PRIVATE_H
