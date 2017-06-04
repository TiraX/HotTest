// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/SceneComponent.h"
#include "OceanTextureComponent.generated.h"

namespace drw
{
	class OceanContext;
	class Ocean;
}

UCLASS(hidecategories = (Object, LOD, Physics, Collision), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class HOUDINIOCEANTEST_API UOceanTextureComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:	
	// Sets default values for this component's properties
	UOceanTextureComponent();
	~UOceanTextureComponent();

	
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

	/**	The height amount of scale for all vertices. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float HeightScale;

	/**	The chopness amount of scale for all vertices. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float ChopScale;

	/**	The speed scale for waves moving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float SpeedScale;

	/**	The normal strenth along xz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float NormalStrength;

	/** Set the procedural texture to obtain ocean data. Size must be 2^x.*/
	UFUNCTION(BlueprintCallable, Category = "Components|OceanTexture")
	void SetTexture(UTexture2D* HeightTexture, UTexture2D* NormalTexture);
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UTexture2D* OceanHeightTexture;
	UTexture2D* OceanNormalTexture;

	void SetupOcean();

	// component to generate fft oceans
	drw::Ocean*	_Ocean;
	drw::OceanContext*	_OceanContext;
	float OceanScale;

	uint8* HeightFieldPixels;
	uint8* NormalPixels;
	float _Time;
};
