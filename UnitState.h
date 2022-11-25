
//---------------------------------------------------------------------------

#ifndef UnitStateH
#define UnitStateH

#include <stdint.h>
#include <vector>
#include "UnitStatePrivate.h"

// These are pointers that track the current state so that the rest of the program can transparently
// access the data without having to go through state.
extern u8* bgPal;
extern u8* nameTable;
extern u8* attrTable;
extern u8* chr;
extern u8* metaSprites;
extern WeakRef<u32> nameTableWidth;
extern WeakRef<u32> nameTableHeight;
extern WeakRef<s32> spriteGridX;
extern WeakRef<s32> spriteGridY;
extern AnsiString* metaSpriteNames;

class State {
public:
    /**
     * Constructs a new State by copying the current state fields.
     */
    State();

    State(const State& other);

    ~State();

    /**
     * Applies up to `count` entries from the Undo History to the current state, applying them going "backward" in the history
     *
     * If `count` is greater than the number of entries in the history, it will only apply up to the number
     * of entries in the history.
     *
     * This does not remove entries from the history, so its possible to Redo immediately after to recover your state.
     */
    void Undo(u32 count);

    /**
     * Applies up to `count` entries from the Undo History to the current state, but applies them going "forward" in the history.
     *
     * If `count` is greater than the number of entries in the history, it will only apply up to the number
     * of entries in the history.
     *
     * This does not remove entries from the history, so its possible to Undo immediately after to recover your state.
     */
    void Redo(u32 count);

    /**
     * Generates a new Undo History item that stores the difference between the current state and the previous state.
     *
     * WARNING: This function will remove any entries from the State that are "newer" than the current state.
     * This means that if you undo 3 times, then call SetUndo, those 3 entries will be removed since the diff can no
     * longer be applied as the base state has changed.
     *
     * This should be called right before making a change to the current state to save the data to the history
     */
    void SetUndo();

    /**
     * Resize the nametable/attrtable to the new width/height
     * Updates the current global state
     */
    void ResizeNameTable(u32 width, u32 height);

    /**
     * Helper function that copies all data from curr into prev
     */
    void CopyCurrentState();

    void SetSpriteGrid(s32 x, s32 y);

    class Values {
    public:
        /**
         * NES raw state data that we track.
         */
        u8 bgPal[4 * 16];
        u8 chr[8192];
        u8 metaSprites[256 * 64 * 4];
        u32 nameTableWidth;
        u32 nameTableHeight;
        s32 spriteGridX;
        s32 spriteGridY;
        std::vector<u8> nameTable;
        std::vector<u8> attrTable;
        AnsiString metaSpriteNames[256];

        // Stores a list of all the values that we are tracking in the state
        std::vector<ValueSerialize::Interface*> fields;

        Values() : bgPal(), chr(), metaSprites(), nameTableWidth(32), nameTableHeight(30),
            spriteGridX(0), spriteGridY(0), nameTable(), attrTable(), metaSpriteNames(), fields() {
            using namespace ValueSerialize;
            fields.push_back(new Fixed<u8, sizeof(bgPal)>(bgPal));
            fields.push_back(new Fixed<u8, sizeof(chr)>(chr));
            fields.push_back(new Fixed<u8, sizeof(metaSprites)>(metaSprites));
            fields.push_back(new Fixed<u32>(&nameTableWidth));
            fields.push_back(new Fixed<u32>(&nameTableHeight));
            fields.push_back(new Fixed<s32>(&spriteGridX));
            fields.push_back(new Fixed<s32>(&spriteGridY));
            fields.push_back(new Resizeable<std::vector<u8> >(&nameTable));
            fields.push_back(new Resizeable<std::vector<u8> >(&attrTable));
            fields.push_back(new Fixed<Resizeable<AnsiString>, 256>(new Resizeable<AnsiString>(metaSpriteNames)));
        }

        ~Values() {
            for (u32 i = 0; i < fields.size(); ++i) {
                delete fields[i];
            }
        }

        inline std::size_t NameSize() {
            return (std::size_t)nameTableWidth * nameTableHeight;
        }

        inline std::size_t AttrSize() {
            return (std::size_t)(nameTableWidth + 3) / 4 * ((std::size_t)(nameTableHeight + 3) / 4);
        }
    };

    /**
     * Stores the current state of the data.
     * The data inside will be pointed to by the global accessors to allow the other code to continue to
     * modify `chr` `bgPal` etc directly without going through the State.
     */
    Values* curr;

    /**
     * Stores the previous state of the data.
     * This is used to create a diff between current and previous when an edit is made, after which
     * the data from current is copied into previous so it can be diffed again when the next edit happens.
     */
    Values* prev;

private:

    /**
     * Applies a single patch to the current state
     */
    void ApplyStateChange(const std::vector<u8>& patch);

    /**
     * Helper function to generate a binary RLE encoded stream and places it into the `out` vector.
     * Returns the number of bytes written
     *
     * The run length encoding on the binary data uses the following approach
     *  - A byte value < 128 indicates a single literal of that value.
     *  - A byte value N between 128 - 255 indicates that the next byte is a run of N - 127 bytes
     */
    std::size_t RLE(std::vector<u8>& out, const std::vector<u8>& data) const;

    /**
     * Helper function to convert from RLE encoded data in the format used above back into the full size data.
     */
    void UnRLE(std::vector<u8>& out, const std::vector<u8>& data) const;

    /**
     * Helper function to add one byte of RLE encoded data to the output vector
     * Returns the number of bytes written
     */
    std::size_t OutputRLEbyte(std::vector<u8>& out, u8 value, u8 count) const;

    /**
     * Stores all of the history items for this undo list. Each Patch contains and RLE encoded
     * diff for each field (or an empty list if there is no change).
     */
    std::vector<std::vector<u8> > undoHistory;
    /**
     * Index into the undo history which tracks what the "current" item is.
     * If you undo, this decreases the index, and redo increases it.
     * This allows for undoing several changes and then redoing forward through them
     */
    u32 undoIndex;
    /**
     * Tracks if SetUndo is called before a change is made. This is used when
     * checking to see if the current state has any changes that need a diff made
     * before calling Undo/Redo
     */
    bool hasChanges;

private:
    /**
     * Helper functions that allows the private methods to be tested
     */
    friend void test_RLE();
    friend void test_grid_sprite();
};



/**
 * Helper function that swaps the global state with a checkpoint.
 * This updates the global `bgPal` `chr` etc pointers to point to the new `global_state` as well.
 */
void SwapGlobalState(State** global, State** checkpoint);

// Main state used for undo tracking tests
extern State* state;
// Create a second state thats used for the checkpointing tests
extern State* checkpoint;


//---------------------------------------------------------------------------
#endif
