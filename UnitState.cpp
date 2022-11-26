
//---------------------------------------------------------------------------


#pragma hdrstop

#include <algorithm>
#include <memory.h>
#include "UnitState.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

State* state;
State* checkpoint;
u8* bgPal;
u8* nameTable;
u8* attrTable;
u8* chr;
u8* metaSprites;
AnsiString* metaSpriteNames;
WeakRef<u32> nameTableWidth;
WeakRef<u32> nameTableHeight;
WeakRef<s32> spriteGridX;
WeakRef<s32> spriteGridY;
static inline u32 NmtIdx(u32 stride, u32 row, u32 col) {
    return row * stride + col;
}

State::State() : curr(nullptr), prev(nullptr), undoHistory(), undoIndex(0), hasChanges(false) {
    curr = new Values;
    prev = new Values;

    curr->nameTableWidth = 32;
    curr->nameTableHeight = 30;

    curr->spriteGridX = 0;
    curr->spriteGridY = 0;

    curr->attrTable.resize(((std::size_t)32 + 3) / 4 + ((std::size_t)30 + 3) / 4);
    curr->nameTable.resize((std::size_t)32 * 30);

    // Create default values for the global state
    curr->nameTableWidth = 32;
    curr->nameTableHeight = 30;
    memset(curr->bgPal,       0,    sizeof(curr->bgPal));
    memset(curr->chr,         0,    sizeof(curr->chr));
    memset(curr->metaSprites, 0xff, sizeof(curr->metaSprites));

    for (int i = 0; i < 256; ++i) {
        curr->metaSpriteNames[i] = "Metasprite " + IntToStr(i);
    }

    CopyCurrentState();
}

State::State(const State& other) {
    // copy all POD fields
    *curr = *other.curr;
    *prev = *other.prev;
    // These are not trivially copyable
    curr->nameTable = other.curr->nameTable;
    prev->nameTable = other.prev->nameTable;
    undoHistory = other.undoHistory;
    undoIndex = other.undoIndex;
    hasChanges = other.hasChanges;
}

State::~State() {
    // i hate that theres no unique_ptr in borland 6
    delete curr;
    delete prev;
}

void State::SetUndo() {
    std::vector<u8> patch;

    // Since we are setting a new history item, if we aren't at the end of the list,
    // we want to clear out all of the items from the current point to the end.
    if (undoIndex < undoHistory.size()) {
        undoHistory.erase(undoHistory.begin() + undoIndex);
    }

    for (u32 i = 0; i < curr->fields.size(); ++i) {
        ValueSerialize::Interface* current = curr->fields[i];
        ValueSerialize::Interface* previous = prev->fields[i];
        current->CreateDiff(patch, previous);
    }

    std::vector<u8> rle_patch;
    RLE(rle_patch, patch);
    undoHistory.push_back(rle_patch);

    // Copy the current values into the previous so we can diff later.
    CopyCurrentState();

    // and now bump the index forward so we know "where" in the undo history we are.
    // we can use this to allow multiple undo and redo in a row
    undoIndex++;
    
    hasChanges = true;
}

void State::Undo(u32 count) {
    // Before we undo, we need to SetUndo once in order to copy the data that has been changed.
    // This is because when a field is changed, the order of operations is SetUndo() -> Update field
    // so in order to undo the latest changes, we need to create the diff in SetUndo before then.
    if (hasChanges) {
        SetUndo();
        hasChanges = false;
    }

    // Now load the history item, and apply the patch to the current data
    while ((count > 0) && undoIndex > 0) {
        // Apply the patch to the current state, and then move the current undo history index back
        // by one to make the previous entry the new "current"
        --undoIndex;
        std::vector<u8> unrle_patch;
        UnRLE(unrle_patch, undoHistory[undoIndex]);
        ApplyStateChange(unrle_patch);
        --count;
    }
}

void State::Redo(u32 count) {
    // Now load the history item, and apply the patch to the current data
    u32 i = 0;
    while ((i < count) && undoIndex < undoHistory.size()) {
        // Apply the patch to the current state and then move the current undo history index forward one
        std::vector<u8> unrle_patch;
        UnRLE(unrle_patch, undoHistory[undoIndex]);
        ApplyStateChange(unrle_patch);
        ++undoIndex;
        ++i;
    }

}

void State::ResizeNameTable(u32 width, u32 height) {
    u32 prevWidth = curr->nameTableWidth;
    u32 prevHeight = curr->nameTableHeight;
    std::vector<u8>& old_nmt = curr->nameTable;

    curr->nameTableWidth = width;
    curr->nameTableHeight = height;

    // now copy the data from current into a new vector with the old nametable placed at the top left corner
    std::vector<u8> new_nmt;
    new_nmt.resize(curr->NameSize());

    u32 w = std::min(width, prevWidth);
    u32 h = std::min(height, prevHeight);
    for (u32 row = 0; row < h; ++row) {
        for (u32 col = 0; col < w; ++col) {
            new_nmt[NmtIdx(width, row, col)] = old_nmt[NmtIdx(prevWidth, row, col)];
        }
    }
    curr->nameTable = new_nmt;
}

void State::ApplyStateChange(const std::vector<u8>& patch) {
    std::size_t patchIdx = 0;
    for (u32 i = 0; i < curr->fields.size(); ++i) {
        ValueSerialize::Interface* current = curr->fields[i];
        current->ApplyDiff(patch, patchIdx);
    }
}

std::size_t State::OutputRLEbyte(std::vector<u8>& out, u8 value, u8 count) const {
    // if the new byte doesn't match, then terminate the previous RLE encode
    if (count == 1) {
        if (value < 128) {
            out.push_back(value);
            return 1;
        } else {
            out.push_back(count + 127);
            out.push_back(value);
            return 2;
        }
    // if our count is about to overflow, then terminate that early too
    }
    out.push_back(count + 127);
    out.push_back(value);
    return 2;
}

/**
 * Perform a run length encoding on the binary data with the following approach
 * A byte value < 128 indicates a single literal of that value.
 * A byte value N between 128 - 255 indicates that the next byte is a run of N - 127 bytes
 */
std::size_t State::RLE(std::vector<u8>& out, const std::vector<u8>& data) const {
    u32 i, count = 0;
    u8 value = data[0];
    std::size_t size = 0;
    for (i = 0; i < data.size(); i++) {
        if (value == data[i] && count < 128) {
            count++;
        }
        else {
            size += OutputRLEbyte(out, value, count);
            value = data[i];
            count = 1;
        }
    }
    return size + OutputRLEbyte(out, value, count);
}

void State::UnRLE(std::vector<u8>& out, const std::vector<u8>& data) const {
    u32 i = 0;
    u8 len, val;

    while (i < data.size()) {
        if (data[i] < 128) {
            // output literal byte
            out.push_back(data[i]);
        }
        else {
            // run length
            len = data[i++] - 127;
            val = data[i];
            for (u8 j = 0; j < len; j++) {
                out.push_back(val);
            }
        }
        i++;
    }
}

void State::CopyCurrentState() {
    memcpy(prev->bgPal,       curr->bgPal,       sizeof(curr->bgPal));
    memcpy(prev->chr,         curr->chr,         sizeof(curr->chr));
    memcpy(prev->metaSprites, curr->metaSprites, sizeof(curr->metaSprites));
    prev->nameTable = curr->nameTable;
    prev->attrTable = curr->attrTable;
    prev->nameTableWidth = curr->nameTableWidth;
    prev->nameTableHeight = curr->nameTableHeight;
    prev->spriteGridX = curr->spriteGridX;
    prev->spriteGridX = curr->spriteGridY;
    for (u32 i = 0; i < 256; ++i) {
        prev->metaSpriteNames[i] = curr->metaSpriteNames[i];
    }
}

void SwapGlobalState(State** global, State** checkpoint) {
    // Swap the two state pointers
    State* tmp = *checkpoint;
    *checkpoint = *global;
    *global = tmp;

    // and then update all the global pointers used for quick access
    bgPal           = tmp->curr->bgPal;
    nameTable       = &tmp->curr->nameTable[0];
    attrTable       = &tmp->curr->attrTable[0];
    chr             = tmp->curr->chr;
    metaSprites     = tmp->curr->metaSprites;
    metaSpriteNames = tmp->curr->metaSpriteNames;
    nameTableWidth.Set(&tmp->curr->nameTableWidth);
    nameTableHeight.Set(&tmp->curr->nameTableHeight);
    spriteGridX.Set(&tmp->curr->spriteGridX);
    spriteGridY.Set(&tmp->curr->spriteGridY);
}

#ifndef __BORLANDC__

// This is data I used to test the State. I built this with MSVC but really it doesn't matter what you use.

#include "acutest.h"

void setup() {
    if (state) {
        delete state;
    }
    if (checkpoint) {
        delete checkpoint;
    }
    state = new State();
    checkpoint = new State();

    bgPal           = state->curr->bgPal;
    nameTable       = &state->curr->nameTable[0];
    attrTable       = &state->curr->attrTable[0];
    chr             = state->curr->chr;
    metaSprites     = state->curr->metaSprites;
    metaSpriteNames = state->curr->metaSpriteNames;
    spriteGridX.Set(&state->curr->spriteGridX);
    spriteGridY.Set(&state->curr->spriteGridY);
    nameTableWidth.Set(&state->curr->nameTableWidth);
    nameTableHeight.Set(&state->curr->nameTableHeight);
}

void test_RLE() {
    setup();
}

void test_grid_sprite() {
    setup();
    state->SetUndo();

    // basic Undo/Redo test for weakref values
    spriteGridX = 100;
    spriteGridY = 200;

    state->Undo(1);
    TEST_CHECK_(spriteGridX == 0, "Sprite Grid X should be reverted to original after Undo: %d expected: %d ", *spriteGridX, 0);

    state->Redo(1);
    TEST_CHECK_(spriteGridX == 100, "Sprite Grid X should be the modified value after Redo: %d expected: %d ", *spriteGridX, 100);

}

void test_palette() {
    setup();
    state->SetUndo();

    // basic Undo/Redo test for palette
    bgPal[2] = 0x10;
    state->Undo(1);
    TEST_CHECK_(bgPal[2] == 0x00, "Palette should be reverted to original after Undo: 0x%02x expected: 0x00 ", bgPal[2]);

    state->Redo(1);
    TEST_CHECK_(bgPal[2] == 0x10, "Palette should be 0x10 after Redo: 0x%02x expected: 0x10 ", bgPal[2]);

    // multiple Undo/Redo test for palette
    // Set all of the values to each palette to their index number in one history item
    // History should contain for bgPal[2] = {0x00, 0x10, 0x02};
    state->SetUndo();
    for (int i = 0; i < sizeof(bgPal); i++) {
        bgPal[i] = i & 0xff;
    }
    TEST_CHECK_(bgPal[2] == 0x02, "Palette should be set to 0x02: %d expected: 0x02 ", bgPal[2]);

    state->Undo(5); // past the end of the undo history
    TEST_CHECK_(bgPal[2] == 0x00, "Palette should be set back to default value 0x00 after complete Undo: 0x%02x expected: 0x00 ", bgPal[2]);

    state->Redo(5); // past the end of the redo history
    TEST_CHECK_(bgPal[2] == 0x02, "Palette should be set back to current value 0x02 after complete Redo: 0x%02x expected: 0x02 ", bgPal[2]);

    state->Undo(1); // check that the previous value of 0x10 is still there
    TEST_CHECK_(bgPal[2] == 0x10, "Palette should be set back to previous value of 0x10 after one Undo: 0x%02x expected: 0x10 ", bgPal[2]);

    state->Redo(1); // check that palette values 
    for (int i = 0; i < sizeof(bgPal); i++) {
        TEST_CHECK_(bgPal[i] == i, "Palette current value should equal its index: 0x%02x expected: 0x%02x ", bgPal[i], i);
    }
}

void test_nametable() {
    setup();
    state->SetUndo();

    // basic Undo/Redo test for nametable
    nameTable[2] = 0x10;
    state->Undo(1);
    TEST_CHECK_(nameTable[2] == 0x00, "NameTable should be reverted to original after Undo: 0x%02x expected: 0x00 ", nameTable[2]);

    state->Redo(1);
    TEST_CHECK_(nameTable[2] == 0x10, "NameTable should be 0x10 after Redo: 0x%02x expected: 0x10 ", nameTable[2]);

    // multiple Undo/Redo test for palette
    // Set all of the values to each palette to their index number in one history item
    // History should contain for bgPal[2] = {0x00, 0x10, 0x02};
    state->SetUndo();
    for (int i = 0; i < state->curr->NameSize(); i++) {
        nameTable[i] = i & 0xff;
    }
    TEST_CHECK_(nameTable[2] == 0x02, "NameTable should be set to 0x02: %d expected: 0x02 ", nameTable[2]);

    state->Undo(5); // past the end of the undo history
    TEST_CHECK_(nameTable[2] == 0x00, "NameTable should be set back to default value 0x00 after complete Undo: 0x%02x expected: 0x00 ", nameTable[2]);

    state->Redo(5); // past the end of the redo history
    TEST_CHECK_(nameTable[2] == 0x02, "NameTable should be set back to current value 0x02 after complete Redo: 0x%02x expected: 0x02 ", nameTable[2]);

    state->Undo(1); // check that the previous value of 0x10 is still there
    TEST_CHECK_(nameTable[2] == 0x10, "NameTable should be set back to previous value of 0x10 after one Undo: 0x%02x expected: 0x10 ", nameTable[2]);

    state->Redo(1); // check that all nametable values are currently their index 
    for (int i = 0; i < state->curr->NameSize(); i++) {
        TEST_CHECK_(nameTable[i] == (i & 0xff), "NameTable current value should equal its index: 0x%02x expected: 0x%02x ", nameTable[i], i & 0xff);
    }

    // Check that resizing nametables properly works with undo
    state->SetUndo();
    state->ResizeNameTable(320, 300);
    TEST_CHECK_(nameTableWidth == 320, "Resized nametableWidth is not updated: %d expected %d", *nameTableWidth, 320);
    TEST_CHECK_(nameTableHeight == 300, "Resized nametableHeight is not updated: %d expected %d", *nameTableHeight, 300);
    // Edit row 2 col 5 to see if it survives the resize from Undo -> Redo
    nameTable[2 * 320 + 5] = 0x25;

    state->Undo(1);
    TEST_CHECK_(nameTableWidth == 32, "Resized nametableWidth is not updated after Undo: %d expected %d", *nameTableWidth, 32);
    TEST_CHECK_(nameTableHeight == 30, "Resized nametableHeight is not updated after Undo: %d expected %d", *nameTableHeight, 30);
    TEST_CHECK_(nameTable[2 * 32 + 5] == 0x00, "Resized nametable after Undo did not revert the value: 0x%02x expected 0x%02x", nameTable[2 * 32 + 5], 0x00);
    state->Redo(1);
    TEST_CHECK_(nameTableWidth == 320, "Resized nametableWidth is not updated after Redo: %d expected %d", *nameTableWidth, 320);
    TEST_CHECK_(nameTableHeight == 300, "Resized nametableHeight is not updated after Redo: %d expected %d", *nameTableHeight, 300);
    TEST_CHECK_(nameTable[2 * 320 + 5] == 0x25, "Resized nametable after Redo did not revert the value: 0x%02x expected 0x%02x", nameTable[2 * 320 + 5], 0x25);

}

void test_checkpoint() {
    setup();
    state->SetUndo();
    
    // Edit the CHR twice to set two different undo points.
    chr[2] = 0x20;
    state->SetUndo();
    chr[3] = 0x30;
    
    // Swap with checkpoint and verify both changes are gone.
    SwapGlobalState(&state, &checkpoint);
    TEST_CHECK_(chr[2] == 0x00, "Swap with checkpoint should Undo change to chr[2]: 0x%02x expected: 0x00 ", chr[2]);
    TEST_CHECK_(chr[3] == 0x00, "Swap with checkpoint should Undo change to chr[3]: 0x%02x expected: 0x00 ", chr[3]);

    // Revert from checkpoint to verify that the two changes are back.
    SwapGlobalState(&state, &checkpoint);
    TEST_CHECK_(chr[2] == 0x20, "Swap with checkpoint should Redo change to chr[2]: 0x%02x expected: 0x20 ", chr[2]);
    TEST_CHECK_(chr[3] == 0x30, "Swap with checkpoint should Redo change to chr[3]: 0x%02x expected: 0x30 ", chr[3]);


}

void test_metaspritename() {
    setup();
    state->SetUndo();

    // basic Undo/Redo test for palette
    metaSpriteNames[2] = "chillin";
    state->Undo(1);
    TEST_CHECK_(metaSpriteNames[2] == "Metasprite 2", "Metasprite Name should be reverted to original after Undo: %s expected: \"Metasprite 2\" ", metaSpriteNames[2].c_str());

    state->Redo(1);
    TEST_CHECK_(metaSpriteNames[2] == "chillin", "Metasprite Name should be 'chillin' after Redo: %s expected: \"chillin\" ", metaSpriteNames[2].c_str());

    // multiple Undo/Redo test for palette
    // Set all of the values to each palette to their index number in one history item
    // History should contain for bgPal[2] = {0x00, 0x10, 0x02};
    state->SetUndo();
    for (int i = 0; i < 256; i++) {
        metaSpriteNames[i] = "changed all " + IntToStr(i);
    }
    TEST_CHECK_(metaSpriteNames[2] == "changed all 2", "Metasprite Name should be set to 'changed all 2': %s expected: \"changed all 2\" ", metaSpriteNames[2].c_str());

    state->Undo(5); // past the end of the undo history
    TEST_CHECK_(metaSpriteNames[2] == "Metasprite 2", "Metasprite Name should be set back to default value after complete Undo: %s expected: \"Metasprite 2\" ", metaSpriteNames[2].c_str());

    state->Redo(5); // past the end of the redo history
    TEST_CHECK_(metaSpriteNames[2] == "changed all 2", "Metasprite Name should be set back to current value 'changed all 2' after complete Redo: %s expected: \"changed all 2\" ", metaSpriteNames[2].c_str());

    state->Undo(1); // check that the previous value of 0x10 is still there
    TEST_CHECK_(metaSpriteNames[2] == "chillin", "Metasprite Name should be set back to previous value of 'chillin' after one Undo: %s expected: \"chillin\" ", metaSpriteNames[2].c_str());

    state->Redo(1); // check that palette values 
    for (int i = 0; i < 256; i++) {
        TEST_CHECK_(metaSpriteNames[i] == "changed all " + IntToStr(i), "Metasprite Name current value should equal its index: %s expected: \"%s\" ", metaSpriteNames[i].c_str(), ("changed all " + IntToStr(i)).c_str());
    }
}

TEST_LIST = {
    { "RLE", test_RLE },
    { "Grid Sprite X/Y", test_grid_sprite },
    { "NameTable", test_nametable },
    { "Checkpoint", test_checkpoint },
    { "Palette", test_palette },
    { "Metasprite Name", test_metaspritename },
    { NULL, NULL }     /* zeroed record marking the end of the list */
};
#endif
