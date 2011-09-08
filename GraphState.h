/*
FMidiAutomation -- A midi automation editor for jack / Linux
Written by Chris Mennie (chris at chrismennie.ca or cmennie at rogers.com)
Copyright (C) 2011 Chris A. Mennie                              

License: Released under the GPL version 3 license. See the included LICENSE.
*/

#ifndef __GRAPHSTATE_H
#define __GRAPHSTATE_H

#include <memory>
#include <set>
#include <map>
#include <vector>

class SequencerEntryBlock;
struct Keyframe;

enum class InsertMode : char;

enum class LineType : char
{
    BarStart,
    BarBeat,
    SubdivisionLine,
    SecondLine,
    ValueLine,
};//LineType

enum class SelectedEntity : char
{
    PointerTickBar,
    LeftTickBar,
    RightTickBar,
    TempoChange,
    SequencerEntrySelection,
    KeyValue,
    InTangent,
    OutTangent,
    Nobody
};//SelectedEntity

enum class DisplayMode : char
{
    Sequencer,
    Curve,
};//DisplayMode

class EntryBlockSelectionState
{
    std::map<std::shared_ptr<SequencerEntryBlock>, int> currentlySelectedEntryOriginalStartTicks;
    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > currentlySelectedEntryBlocks;
    std::set<std::shared_ptr<SequencerEntryBlock> > origSelectedEntryBlocks; //for rubberbanding

public:
    EntryBlockSelectionState() {}
    ~EntryBlockSelectionState() {}

    bool HasSelected();
    bool IsSelected(std::shared_ptr<SequencerEntryBlock> entryBlock);
    bool IsOrigSelected(std::shared_ptr<SequencerEntryBlock> entryBlock); //checks origSelectedEntryBlocks
    void ClearSelected();
    void ResetRubberbandingSelection();

    int GetNumSelected();
    std::shared_ptr<SequencerEntryBlock> GetFirstEntryBlock();
    int GetOriginalStartTick(std::shared_ptr<SequencerEntryBlock> entryBlock);

    std::multimap<int, std::shared_ptr<SequencerEntryBlock> > GetEntryBlocksMapCopy();
    std::map<std::shared_ptr<SequencerEntryBlock>, int> GetEntryOriginalStartTicksCopy();
    std::set<std::shared_ptr<SequencerEntryBlock> > GetOrigSelectedEntryBlocksCopy();

    std::pair<decltype(currentlySelectedEntryBlocks.begin()), decltype(currentlySelectedEntryBlocks.end())> GetCurrentlySelectedEntryBlocks();

    void SetCurrentlySelectedEntryOriginalStartTicks(std::map<std::shared_ptr<SequencerEntryBlock>, int> &origStartTicks); //FIXME: This feels very questionable

    void AddSelectedEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock);
    void RemoveSelectedEntryBlock(std::shared_ptr<SequencerEntryBlock> entryBlock);
};//EntryBlockSelectionState

class KeyframeSelectionState
{
    std::shared_ptr<Keyframe> selectedKey; //mostly useful for who owns the selected tangent grab handle
    std::map<int, std::shared_ptr<Keyframe> > currentlySelectedKeyframes; //not a multimap since keys at ticks are unique
    std::set<std::shared_ptr<Keyframe> > origSelectedKeyframes;
    std::map<std::shared_ptr<Keyframe>, int> movingKeyOrigTicks;
    std::map<std::shared_ptr<Keyframe>, double> movingKeyOrigValues;    

public:
    KeyframeSelectionState() {}
    ~KeyframeSelectionState() {}

    bool HasSelected();
    void ClearSelectedKeyframes();
    void ResetRubberbandingSelection();
    bool IsSelected(std::shared_ptr<Keyframe> keyframe);
    bool IsOrigSelected(std::shared_ptr<Keyframe> keyframe); //checks origSelectedKeyframes
    int GetNumSelected();
    std::shared_ptr<Keyframe> GetFirstKeyframe();
    int GetOrigTick(std::shared_ptr<Keyframe> keyframe);
    double GetOrigValue(std::shared_ptr<Keyframe> keyframe);    

    std::pair<decltype(currentlySelectedKeyframes.begin()), decltype(currentlySelectedKeyframes.end())> GetCurrentlySelectedKeyframes();

    std::map<int, std::shared_ptr<Keyframe> > GetSelectedKeyframesCopy();

    void SetCurrentlySelectedKeyframes(std::map<int, std::shared_ptr<Keyframe> > &origSelectedKeyframes); //FIXME: This feels very questionable

    void AddKeyframe(std::shared_ptr<Keyframe> keyframe);
    void AddOrigKeyframe(std::shared_ptr<Keyframe> keyframe);
    void RemoveKeyframe(std::shared_ptr<Keyframe> keyframe);
};//KeyframeSelectionState

struct GraphState
{    
    double baseOffsetX; //when actively scrolling
    double baseOffsetY;
    bool inMotion; //when actively scrolling

    int zeroithTickPixel;
    double offsetX; //scroll offset
    double offsetY;
    int barsSubdivisionAmount;
    int ticksPerPixel; //negative means N pixels per tick
    double valuesPerPixel;
    std::vector<std::pair<unsigned int, LineType> > verticalLines;
    std::vector<std::pair<unsigned int, std::string> > upperLineText;
    std::vector<std::pair<unsigned int, LineType> > horizontalLines;
    std::vector<std::pair<unsigned int, std::string> > valueLineText;

    std::vector<int> verticalPixelTickValues;
    std::vector<double> horizontalPixelValues;
    std::vector<int> roundedHorizontalValues;

    int curMousePosX;
    int curMousePosY;

    //Time at which the pointer is at
    int curPointerTick;
    int curPointerTickXPixel; //how far over is it?   

    int leftMarkerTick;
    int rightMarkerTick;
    int leftMarkerTickXPixel;
    int rightMarkerTickXPixel;

    SelectedEntity selectedEntity;
    EntryBlockSelectionState entryBlockSelectionState;
    KeyframeSelectionState keyframeSelectionState;

    bool didMoveKey;
    bool didMoveKeyOutTangent;
    bool didMoveKeyInTangent;

    DisplayMode displayMode;
    int lastSequencerPointerTick; //for swaping back to the seqeucner
    int lastSequencerLeftPointerTick;
    int lastSequencerRightPointerTick;

    bool doingRubberBanding;
    InsertMode insertMode;
    
    GraphState();
    ~GraphState();
    void doInit();

    void refreshVerticalLines(unsigned int areaWidth, unsigned int areaHeight);
    void refreshHorizontalLines(unsigned int areaWidth, unsigned int areaHeight);
    void setOffsetCenteredOnTick(int tick, int drawingAreaWidth);
    void setOffsetCenteredOnValue(double value, int drawingAreaHeight);

    template<class Archive> void serialize(Archive &ar, const unsigned int version);
    friend class boost::serialization::access;
};//GraphState


BOOST_CLASS_VERSION(GraphState, 1);

#endif

