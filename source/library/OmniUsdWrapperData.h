// SPDX-FileCopyrightText: Copyright (c) 2021-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.
#pragma once

#include "UsdIncludes.h"

TF_DECLARE_WEAK_AND_REF_PTRS(OmniUsdWrapperData);

class OmniUsdWrapperData : public SdfAbstractData
{
public:
    OmniUsdWrapperData(SdfLayerRefPtr wrappedLayer, SdfAbstractDataRefPtr wrappedData)
        : _wrappedLayer(wrappedLayer), _wrappedData(wrappedData)
    {
    }

    SdfLayerRefPtr GetWrappedLayer() const
    {
        return _wrappedLayer;
    }

    SdfAbstractDataRefPtr GetWrappedData() const
    {
        return _wrappedData;
    }

    void SetWrappedData(SdfAbstractDataRefPtr wrappedData)
    {
        _wrappedData = wrappedData;
    }

    /// SdfAbstractData overrides
    virtual bool StreamsData() const override
    {
        return _wrappedData->StreamsData();
    }

    virtual void CopyFrom(const SdfAbstractDataConstPtr& source) override
    {
        _wrappedData->CopyFrom(source);
    }
    virtual void CreateSpec(const SdfPath& specPath, SdfSpecType specType) override
    {
        _wrappedData->CreateSpec(specPath, specType);
    }
    virtual bool HasSpec(const SdfPath& specPath) const override
    {
        return _wrappedData->HasSpec(specPath);
    }
    virtual void EraseSpec(const SdfPath& specPath) override
    {
        _wrappedData->EraseSpec(specPath);
    }
    virtual void MoveSpec(const SdfPath& oldPath, const SdfPath& newPath) override
    {
        _wrappedData->MoveSpec(oldPath, newPath);
    }

    virtual SdfSpecType GetSpecType(const SdfPath& specPath) const override
    {
        return _wrappedData->GetSpecType(specPath);
    }
    virtual bool Has(const SdfPath& specPath, const TfToken& fieldName, SdfAbstractDataValue* value) const override
    {
        return _wrappedData->Has(specPath, fieldName, value);
    }
    virtual bool Has(const SdfPath& specPath, const TfToken& fieldName, VtValue* value = NULL) const override
    {
        return _wrappedData->Has(specPath, fieldName, value);
    }
    virtual VtValue Get(const SdfPath& specPath, const TfToken& fieldName) const override
    {
        return _wrappedData->Get(specPath, fieldName);
    }
    virtual void Set(const SdfPath& specPath, const TfToken& fieldName, const VtValue& value) override
    {
        _wrappedData->Set(specPath, fieldName, value);
    }
    virtual void Set(const SdfPath& specPath, const TfToken& fieldName, const SdfAbstractDataConstValue& value) override
    {
        _wrappedData->Set(specPath, fieldName, value);
    }
    virtual void Erase(const SdfPath& specPath, const TfToken& fieldName) override
    {
        _wrappedData->Erase(specPath, fieldName);
    }
    virtual std::vector<TfToken> List(const SdfPath& specPath) const override
    {
        return _wrappedData->List(specPath);
    }

    virtual std::set<double> ListAllTimeSamples() const override
    {
        return _wrappedData->ListAllTimeSamples();
    }
    virtual std::set<double> ListTimeSamplesForPath(const SdfPath& specPath) const override
    {
        return _wrappedData->ListTimeSamplesForPath(specPath);
    }
    virtual bool GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const override
    {
        return _wrappedData->GetBracketingTimeSamples(time, tLower, tUpper);
    }
    virtual size_t GetNumTimeSamplesForPath(const SdfPath& specPath) const override
    {
        return _wrappedData->GetNumTimeSamplesForPath(specPath);
    }
    virtual bool GetBracketingTimeSamplesForPath(const SdfPath& specPath,
                                                 double time,
                                                 double* tLower,
                                                 double* tUpper) const override
    {
        return _wrappedData->GetBracketingTimeSamplesForPath(specPath, time, tLower, tUpper);
    }
    virtual bool QueryTimeSample(const SdfPath& specPath, double time, SdfAbstractDataValue* optionalValue) const override
    {
        return _wrappedData->QueryTimeSample(specPath, time, optionalValue);
    }
    virtual bool QueryTimeSample(const SdfPath& specPath, double time, VtValue* value) const override
    {
        return _wrappedData->QueryTimeSample(specPath, time, value);
    }
    virtual void SetTimeSample(const SdfPath& specPath, double time, const VtValue& value) override
    {
        _wrappedData->SetTimeSample(specPath, time, value);
    }
    virtual void EraseTimeSample(const SdfPath& specPath, double time) override
    {
        _wrappedData->EraseTimeSample(specPath, time);
    }

protected:
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const override
    {
        _wrappedData->VisitSpecs(visitor);
    }

private:
    SdfLayerRefPtr _wrappedLayer;
    SdfAbstractDataRefPtr _wrappedData;
};
