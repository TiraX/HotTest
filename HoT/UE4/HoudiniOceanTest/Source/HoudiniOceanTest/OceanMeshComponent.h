// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/MeshComponent.h"
#include "OceanMeshComponent.generated.h"

class FPrimitiveSceneProxy;

namespace drw
{
	class OceanContext;
	class Ocean;
}

/**
 * create ocean surface with FFT from Houdini Ocean Toolkit source
 */
UCLASS(hidecategories = (Object, LOD, Physics, Collision), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class HOUDINIOCEANTEST_API UOceanMeshComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()

public:
	~UOceanMeshComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int	GridRes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bChop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bJacobian;

	/** Affects the shape of the waves */ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WindSpeed;	

	/** Affects the direction the waves travel in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WindDirection;

	/** This is used to set the so called “A” fiddle factor in the Tessendorf paper. The waves are scaled so that they will be roughly less than this height (strictly so for the t=0 timestep). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WaveHeight;

	/** Waves below this length will be filtered out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ShortestWaveLength;

	/** In a “fully developed” ocean you will have waves travelling in both the forward and backwards directions. This parameter damps out the negative direcion waves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damp;	

	/** Controls how closely the waves travel in the direction of the wind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int WindAlign;

	/** Affects the spectrum of waves generated. Visually in doesn’t seem to have that great an influence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Depth;

	/**	The amount of chop displacenemnt that is applied to the input points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Choppyness;
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


	friend class FOceanMeshSceneProxy;

	drw::Ocean*	_Ocean;
	drw::OceanContext*	_OceanContext;
};