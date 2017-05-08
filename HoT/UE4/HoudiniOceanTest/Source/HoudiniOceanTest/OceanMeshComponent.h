// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/MeshComponent.h"
#include "OceanMeshComponent.generated.h"

class FPrimitiveSceneProxy;

/**
 * create ocean surface with FFT from Houdini Ocean Toolkit source
 */
UCLASS(hidecategories = (Object, LOD, Physics, Collision), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class HOUDINIOCEANTEST_API UOceanMeshComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()

private:

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.


	friend class FCustomMeshSceneProxy;	
};