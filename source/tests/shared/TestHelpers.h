// SPDX-FileCopyrightText: Copyright (c) 2018-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "UsdIncludes.h"

#include <iostream>
#include <random>
#include <utility>

#define DEBUG_OUTPUT_PATHS 0

std::default_random_engine gRandomEngine(1515);


struct Range
{
    int min;
    int max;
};

void GenerateRandomLayerContent(PXR_NS::SdfLayerRefPtr layer,
                                int numLevels,
                                const Range* primPerLevelArray,
                                const Range& attrPerPrim,
                                bool isBenchmarkingEdits = false,
                                const PXR_NS::SdfPath& parentPath = PXR_NS::SdfPath::AbsoluteRootPath(),
                                std::string const& primPrefix = "p")
{
    PXR_NAMESPACE_USING_DIRECTIVE;

    const Range& primPerLevel = primPerLevelArray[0];

    auto parentPrim =
        (parentPath == SdfPath::AbsoluteRootPath()) ? layer->GetPseudoRoot() : layer->GetPrimAtPath(parentPath);

    const int numPrim = std::uniform_int_distribution<int>(primPerLevel.min, primPerLevel.max)(gRandomEngine);
    for (int primIdx = 0; primIdx < numPrim; ++primIdx)
    {
        std::string primName(primPrefix);
        primName.append("_").append(std::to_string(primIdx));

        // XXX:aluk
        // When authoring in Sdf, there is no API to get the usdPrimTypeName for a schema, so we have to hardcode
        // "Sphere" here.
        auto prim = SdfPrimSpec::New(parentPrim, primName, SdfSpecifierDef, "Sphere");

        const int numAttr = std::uniform_int_distribution<int>(attrPerPrim.min, attrPerPrim.max)(gRandomEngine);
        for (int attrIdx = 0; attrIdx < numAttr; ++attrIdx)
        {
            std::string attrName =
                isBenchmarkingEdits ? UsdGeomTokens->radius : primName + "_a_" + std::to_string(attrIdx);

            auto attr = SdfAttributeSpec::New(prim, attrName, SdfValueTypeNames->Double);
        }

        if (numLevels > 1)
        {
            GenerateRandomLayerContent(layer, numLevels - 1, primPerLevelArray + 1, attrPerPrim, isBenchmarkingEdits,
                                       prim->GetPath(), primName);
        }
    }
}


template <typename ChildPolicy>
void MoveObj(PXR_NS::SdfLayerHandle layer, const PXR_NS::SdfPath sourcePath, const PXR_NS::SdfPath targetPath)
{
    auto movingObj = layer->GetObjectAtPath(sourcePath);
    assert(movingObj);

    const PXR_NS::TfToken sourceName = sourcePath.GetNameToken();
    const PXR_NS::TfToken targetName = targetPath.GetNameToken();
    bool bNeedRename = (sourceName != targetName);
    if (bNeedRename)
    {
        const bool bCanRenameSrc = !layer->HasSpec(sourcePath.ReplaceName(targetName));
        const bool bCanRenameDst = !layer->HasSpec(targetPath.ReplaceName(sourceName));
        if (bCanRenameSrc | !bCanRenameDst)
        {
            PXR_NS::TfToken newName = targetName;
            bNeedRename = !bCanRenameSrc;
            if (bNeedRename)
            {
                newName = PXR_NS::TfToken(newName.GetString() + "__RENAME__");
            }

            // rename
            PXR_NS::Sdf_ChildrenUtils<ChildPolicy>::Rename(movingObj.GetSpec(), newName);
        }
    }
    {
        // move
        PXR_NS::Sdf_ChildrenUtils<ChildPolicy>::InsertChild(
            layer, ChildPolicy::GetParentPath(targetPath),
            PXR_NS::TfStatic_cast<typename ChildPolicy::ValueType>(movingObj), -1);
    }
    if (bNeedRename)
    {
        // rename
        PXR_NS::Sdf_ChildrenUtils<ChildPolicy>::Rename(movingObj.GetSpec(), targetName);
    }
}

class RandomChangeGenerator
{
    PXR_NS::SdfLayerRefPtr _layer;
    std::vector<PXR_NS::SdfPath> _flattenPathTree;
    PXR_NS::SdfPath _tempPath;
    int _createIdx;
    bool _only1Level;

    PXR_NS::SdfPath _SelectTargetParentPath()
    {
        while (true)
        {
            const size_t treeIdx =
                _only1Level ? 0 : std::uniform_int_distribution<size_t>(0, _flattenPathTree.size() - 1)(gRandomEngine);
            const PXR_NS::SdfPath path = _flattenPathTree[treeIdx];
            if (path.IsAbsoluteRootOrPrimPath())
            {
                return path;
            }
        }
    }

    template <typename Set>
    PXR_NS::SdfPath _SelectSourcePath(const PXR_NS::SdfPath targetParentPath, const Set& ignorePathSet)
    {
        if (_flattenPathTree.size() > 1)
        {
            for (int i = 0; i < 100; ++i)
            {
                const size_t treeIdx =
                    std::uniform_int_distribution<size_t>(1, _flattenPathTree.size() - 1)(gRandomEngine);
                const PXR_NS::SdfPath path = _flattenPathTree[treeIdx];
                if (path.IsPrimPath() && ignorePathSet.find(path) == ignorePathSet.end() &&
                    !path.HasPrefix(_tempPath) && !targetParentPath.HasPrefix(path))
                {
                    return path;
                }
            }
        }
        return PXR_NS::SdfPath();
    }

    void _DebugDump()
    {
        for (size_t i = 0; i < _flattenPathTree.size(); ++i)
        {
            std::cout << "\t" << _flattenPathTree[i] << "\n";
        }
    }

    void _MovePath(const PXR_NS::SdfPath sourcePath, const PXR_NS::SdfPath targetPath)
    {
        const auto sourceIt = std::lower_bound(_flattenPathTree.begin(), _flattenPathTree.end(), sourcePath);
        assert(sourceIt != _flattenPathTree.end() && *sourceIt == sourcePath);
        const auto sourceChildBegIt = _flattenPathTree.erase(sourceIt);
        const auto sourceChildEndIt =
            std::find_if_not(sourceChildBegIt, _flattenPathTree.end(),
                             [=](const PXR_NS::SdfPath& path) { return path.HasPrefix(sourcePath); });

        auto sourceChildStart = sourceChildBegIt - _flattenPathTree.begin();
        auto sourceChildCount = sourceChildEndIt - sourceChildBegIt;

        auto targetIt = std::lower_bound(_flattenPathTree.begin(), _flattenPathTree.end(), targetPath);
        assert(targetIt == _flattenPathTree.end() || *targetIt != targetPath);
        targetIt = _flattenPathTree.insert(targetIt, 1 + sourceChildCount, PXR_NS::SdfPath());
        *targetIt = targetPath;
        if (sourceChildCount > 0)
        {
            const auto targetIdx = targetIt - _flattenPathTree.begin();
            if (targetIdx <= sourceChildStart)
            {
                sourceChildStart += (1 + sourceChildCount);
            }

            ++targetIt;
            auto sourceChildIt = _flattenPathTree.begin() + sourceChildStart;
            for (auto i = 0; i < sourceChildCount; ++i)
            {
                *targetIt++ = (*sourceChildIt++).ReplacePrefix(sourcePath, targetPath, false);
            }

            _flattenPathTree.erase(sourceChildIt - sourceChildCount, sourceChildIt);
        }

#if DEBUG_OUTPUT_PATHS
        _DebugDump();
#endif
    }

    template <typename ChildPolicy>
    void _MoveObj(const PXR_NS::SdfPath sourcePath, const PXR_NS::SdfPath targetParentPath, const PXR_NS::TfToken targetName)
    {
        const PXR_NS::SdfPath targetPath = ChildPolicy::GetChildPath(targetParentPath, targetName);
#if DEBUG_OUTPUT
        std::cout << "--- move: " << sourcePath << " -> " << targetPath << std::endl;
#endif
        MoveObj<ChildPolicy>(_layer, sourcePath, targetPath);
        _MovePath(sourcePath, targetPath);
    }

    void _ErasePath(PXR_NS::SdfPath erasePath)
    {
        const auto eraseIt = std::lower_bound(_flattenPathTree.begin(), _flattenPathTree.end(), erasePath);
        assert(eraseIt != _flattenPathTree.end() && *eraseIt == erasePath);
        const auto eraseEndIt = std::find_if_not(
            eraseIt + 1, _flattenPathTree.end(), [=](const PXR_NS::SdfPath& path) { return path.HasPrefix(erasePath); });

        _flattenPathTree.erase(eraseIt, eraseEndIt);
    }

    void _CreatePath(PXR_NS::SdfPath createPath)
    {
        const auto createIt = std::lower_bound(_flattenPathTree.begin(), _flattenPathTree.end(), createPath);
        assert(createIt == _flattenPathTree.end() || *createIt != createPath);

        _flattenPathTree.insert(createIt, createPath);
    }

public:
    RandomChangeGenerator() : _createIdx(0), _only1Level(false)
    {
        _tempPath = PXR_NS::SdfPath("/__TEMP__");

#if DEBUG_OUTPUT_PATHS
        _DebugDump();
#endif
    }

    void SetLayer(PXR_NS::SdfLayerRefPtr layer)
    {
        _layer = layer;
    }
    PXR_NS::SdfLayerRefPtr GetLayer() const
    {
        return _layer;
    }

    void ExecuteUpdate()
    {
        const size_t treeIdx = std::uniform_int_distribution<size_t>(0, _flattenPathTree.size() - 1)(gRandomEngine);
        auto path = _flattenPathTree[treeIdx];
        auto obj = _layer->GetObjectAtPath(path);
        if (obj == nullptr)
        {
            std::cout << "ExecuteUpdate: can't get object at <" << path << ">\n";
            assert(0);
        }

        if (obj->GetSpecType() == PXR_NS::SdfSpecTypeAttribute)
        {
            const int timeIdx = std::uniform_int_distribution<int>(1, 5)(gRandomEngine);
            if (std::uniform_int_distribution<int>(0, 2)(gRandomEngine) != 0)
            {
                static double timeSampleCount = 0.0;
                _layer->SetTimeSample(path, double(timeIdx), PXR_NS::VtValue(timeSampleCount));
                timeSampleCount += 1.0;
            }
            else
            {
                _layer->EraseTimeSample(path, double(timeIdx));
            }

            static double defaultCount = 0.0;
            obj->SetField(PXR_NS::SdfFieldKeys->Default, PXR_NS::VtValue(defaultCount));
            defaultCount += 1.0;
        }
        else
        {
            const int fieldIdx = std::uniform_int_distribution<int>(1, 5)(gRandomEngine);
            const auto fieldName = PXR_NS::TfToken("f_" + std::to_string(fieldIdx));
            PXR_NS::VtValue value(std::uniform_int_distribution<int>(1, 1000)(gRandomEngine));
#if DEBUG_OUTPUT
            printf("--- set <%s> %s = %s\n", path.GetText(), fieldName.GetText(), PXR_NS::TfStringify(value).c_str());
#endif
            obj->SetField(fieldName, value);
        }
    }

    void ExecuteErase()
    {
        if (_flattenPathTree.size() > 1)
        {
            const size_t treeIdx = std::uniform_int_distribution<size_t>(1, _flattenPathTree.size() - 1)(gRandomEngine);
            const PXR_NS::SdfPath path = _flattenPathTree[treeIdx];

#if DEBUG_OUTPUT
            printf("--- erase <%s>\n", path.GetText());
#endif
            if (PXR_NS::Sdf_ChildrenUtils<PXR_NS::Sdf_PrimChildPolicy>::RemoveChild(
                    _layer, PXR_NS::Sdf_PrimChildPolicy::GetParentPath(path),
                    PXR_NS::Sdf_PrimChildPolicy::GetFieldValue(path)))
            {
                _ErasePath(path);
            }
        }
    }

    void ExecuteCreate()
    {
        auto parentPath = _SelectTargetParentPath();

        PXR_NS::SdfPath childPath;
        while (true)
        {
            auto childName = PXR_NS::TfToken("p_C" + std::to_string(_createIdx++));
            childPath = PXR_NS::Sdf_PrimChildPolicy::GetChildPath(parentPath, childName);

            if (!_layer->HasSpec(childPath))
            {
                break;
            }
        }
#if DEBUG_OUTPUT
        printf("--- create <%s>\n", childPath.GetText());
#endif
        PXR_NS::Sdf_ChildrenUtils<PXR_NS::Sdf_PrimChildPolicy>::CreateSpec(
            _layer, childPath, PXR_NS::SdfSpecTypePrim, false);

        _CreatePath(childPath);
    }

    void ExecuteMoveSequence(int minMoves, int maxMoves)
    {
        const int isLoop = std::uniform_int_distribution<int>(0, 1)(gRandomEngine);
        const int numMoves = std::uniform_int_distribution<int>(minMoves, maxMoves)(gRandomEngine);

        std::unordered_set<PXR_NS::SdfPath, PXR_NS::SdfPath::Hash> ignorePathSet;

        PXR_NS::SdfPath targetParentPath;
        PXR_NS::TfToken targetName;
        if (isLoop)
        {
            targetParentPath = _tempPath.GetParentPath();
            targetName = _tempPath.GetNameToken();
        }
        else
        {
            targetParentPath = _SelectTargetParentPath();
        }
        for (int i = 0; i < numMoves; ++i)
        {
            const PXR_NS::SdfPath sourcePath = _SelectSourcePath(targetParentPath, ignorePathSet);
            if (sourcePath.IsEmpty())
            {
                break;
            }

            const PXR_NS::SdfPath sourceParentPath = sourcePath.GetParentPath();
            const PXR_NS::TfToken sourceName = sourcePath.GetNameToken();
            if (targetName.IsEmpty())
            {
                targetName = sourceName;
                while (_layer->HasSpec(targetParentPath.AppendChild(targetName)))
                {
                    targetName = PXR_NS::TfToken(targetName.GetString() + "R");
                }
            }
            const PXR_NS::SdfPath targetPath = PXR_NS::Sdf_PrimChildPolicy::GetChildPath(targetParentPath, targetName);
            ignorePathSet.insert(targetPath);

            _MoveObj<PXR_NS::Sdf_PrimChildPolicy>(sourcePath, targetParentPath, targetName);

            targetParentPath = sourceParentPath;
            targetName = sourceName;
        }

        if (isLoop && targetName != _tempPath.GetNameToken())
        {
            _MoveObj<PXR_NS::Sdf_PrimChildPolicy>(_tempPath, targetParentPath, targetName);
        }
    }

    void ExecuteReorder()
    {
        for (int i = 0; i < 10; ++i)
        {
            const size_t treeIdx = std::uniform_int_distribution<size_t>(0, _flattenPathTree.size() - 1)(gRandomEngine);
            auto specPath = _flattenPathTree[treeIdx];
            PXR_NS::VtValue childrenValue = _layer->GetField(specPath, PXR_NS::SdfChildrenKeys->PrimChildren);
            if (childrenValue.IsEmpty())
            {
                continue;
            }
            const auto& childrenVec = childrenValue.Get<PXR_NS::TfTokenVector>();
            auto childrenCount = childrenVec.size();
            if (childrenCount < 2)
            {
                continue;
            }

            // PXR_NS::SdfPathVector outChildrenVec(childrenCount);
            std::vector<size_t> reindexVec(childrenCount);
            for (size_t i = 0; i < childrenCount; ++i)
            {
                reindexVec[i] = i;
            }

            for (size_t i = 0; i < childrenCount - 1; i++)
            {
                auto j = std::uniform_int_distribution<size_t>(i + 1, childrenCount - 1)(gRandomEngine);
                std::swap(reindexVec[i], reindexVec[j]);
            }

            PXR_NS::SdfPrimSpecHandleVector specChildrenVec(childrenCount);
            for (size_t i = 0; i < childrenCount; ++i)
            {
                auto childName = childrenVec[reindexVec[i]];
                auto childPath = PXR_NS::Sdf_PrimChildPolicy::GetChildPath(specPath, childName);
                specChildrenVec[i] = _layer->GetPrimAtPath(childPath);
            }

            PXR_NS::Sdf_ChildrenUtils<PXR_NS::Sdf_PrimChildPolicy>::SetChildren(_layer, specPath, specChildrenVec);
            break;
        }
    }

    void Execute(int min2Update,
                 int max2Update,
                 int min2Erase,
                 int max2Erase,
                 int min2Create,
                 int max2Create,
                 int min2Move,
                 int max2Move,
                 int minMovesInSeq,
                 int maxMovesInSeq,
                 int min2Reorder,
                 int max2Reorder)
    {
        _flattenPathTree.clear();
        _layer->Traverse(PXR_NS::SdfPath::AbsoluteRootPath(),
                         [this](const PXR_NS::SdfPath& path)
                         {
                             // if (path.IsAbsoluteRootOrPrimPath())
                             {
                                 if (!_layer->HasSpec(path))
                                 {
                                     printf("--- spec <%s> is missing\n", path.GetText());
                                     assert(0);
                                 }
                                 _flattenPathTree.push_back(path);
                             }
                         });
        std::sort(_flattenPathTree.begin(), _flattenPathTree.end());

        int num2Update = std::uniform_int_distribution<int>(min2Update, max2Update)(gRandomEngine);
        int num2Erase = std::uniform_int_distribution<int>(min2Erase, max2Erase)(gRandomEngine);
        int num2Create = std::uniform_int_distribution<int>(min2Create, max2Create)(gRandomEngine);
        int num2Move = std::uniform_int_distribution<int>(min2Move, max2Move)(gRandomEngine);
        int num2Reorder = std::uniform_int_distribution<int>(min2Reorder, max2Reorder)(gRandomEngine);

        for (int numTotal = num2Update + num2Erase + num2Create + num2Move + num2Reorder; numTotal > 0; --numTotal)
        {
            int idx = std::uniform_int_distribution<int>(0, numTotal - 1)(gRandomEngine);

            if (idx < num2Update)
            {
                ExecuteUpdate();
                --num2Update;
                continue;
            }
            idx -= num2Update;

            if (idx < num2Erase)
            {
                ExecuteErase();
                --num2Erase;
                continue;
            }
            idx -= num2Erase;

            if (idx < num2Create)
            {
                ExecuteCreate();
                --num2Create;
                continue;
            }
            idx -= num2Create;

            if (idx < num2Move)
            {
                ExecuteMoveSequence(minMovesInSeq, maxMovesInSeq);
                --num2Move;
                continue;
            }
            idx -= num2Move;

            assert(idx < num2Reorder);
            ExecuteReorder();
            --num2Reorder;
        }
    }
};


template <typename T>
class UsdNoticeListener : public pxr::TfWeakBase
{
public:
    virtual ~UsdNoticeListener()
    {
        revokeListener();
    }

    void registerListener()
    {
        m_usdNoticeListenerKey = pxr::TfNotice::Register(pxr::TfCreateWeakPtr(this), &UsdNoticeListener::handleNotice);
    }

    void revokeListener()
    {
        if (m_usdNoticeListenerKey.IsValid())
        {
            pxr::TfNotice::Revoke(m_usdNoticeListenerKey);
        }
    }

    virtual void handleNotice(const T& objectsChanged) = 0;

private:
    pxr::TfNotice::Key m_usdNoticeListenerKey;
};
