// Fill out your copyright notice in the Description page of Project Settings.

#include "HoudiniOceanTest.h"
#include "OceanTextureComponent.h"
#include "Ocean.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOceanTextureComponent, Verbose, All);
DEFINE_LOG_CATEGORY(LogOceanTextureComponent);

// Sets default values for this component's properties
UOceanTextureComponent::UOceanTextureComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, OceanHeightTexture(nullptr)
	, OceanNormalTexture(nullptr)
	, HeightFieldPixels(nullptr)
	, NormalPixels(nullptr)
	, _Ocean(nullptr)
	, _OceanContext(nullptr)
	, bChop(true)
	, bJacobian(true)
	, WindSpeed(30.f)
	, WindDirection(0.f)
	, WaveHeight(50.f)
	, ShortestWaveLength(0.05f)
	, Damp(0.6f)
	, WindAlign(2)
	, Depth(200.f)
	, Choppyness(1.6f)
	, WaveScale(1.f)
	, OceanScale(0.1f)
	, _Time(0.f)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	bTickInEditor = true;

	// ...
}

UOceanTextureComponent::~UOceanTextureComponent()
{
	if (_Ocean)
		delete _Ocean;
	if (_OceanContext)
		delete _OceanContext;
	if (HeightFieldPixels)
		delete[] HeightFieldPixels;
	if (NormalPixels)
		delete[] NormalPixels;
}

void UOceanTextureComponent::SetTexture(UTexture2D* HeightTexture, UTexture2D* NormalTexture)
{
	if (HeightTexture == nullptr || NormalTexture == nullptr)
		return;
	if (HeightTexture->GetSizeX() != NormalTexture->GetSizeX())
		return;

	check(HeightTexture->GetSizeX() == HeightTexture->GetSizeY());
	check(NormalTexture->GetSizeX() == NormalTexture->GetSizeY());

	OceanHeightTexture = HeightTexture;
	OceanNormalTexture = NormalTexture;
	SetupOcean();
}

void UOceanTextureComponent::SetupOcean()
{
	if (OceanHeightTexture == nullptr || OceanNormalTexture == nullptr)
		return;

	if (_Ocean)
		delete _Ocean;
	if (_OceanContext)
		delete _OceanContext;
	if (HeightFieldPixels)
		delete[] HeightFieldPixels;
	if (NormalPixels)
		delete[] NormalPixels;

	// create things
	int gridres = OceanHeightTexture->GetSizeX();

	float stepsize = 1.0f;// GridSize / (float)gridres;
	_Ocean = new drw::Ocean(gridres, gridres, stepsize, stepsize,
		WindSpeed, ShortestWaveLength, WaveHeight, WindDirection,
		Damp, WindAlign, Depth, 12306);
	OceanScale = _Ocean->get_height_normalize_factor();

	_OceanContext = _Ocean->new_context(true, bChop, true, bJacobian);

	HeightFieldPixels = new uint8[gridres * gridres * 4];
	memset(HeightFieldPixels, 0, gridres * gridres * 4);
	NormalPixels = new uint8[gridres * gridres * 4];
	memset(NormalPixels, 0, gridres * gridres * 4);

	_Time = 0.f;
}

// Called when the game starts
void UOceanTextureComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData)
{
	if (Texture->Resource)
	{
		struct FUpdateTextureRegionsData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			FUpdateTextureRegion2D* Region;
			uint32 SrcPitch;
			uint32 SrcBpp;
			uint8* SrcData;
		};

		FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D;
		Region->SrcX = 0;
		Region->SrcY = 0;
		Region->DestX = 0;
		Region->DestY = 0;
		Region->Width = Texture->GetSizeX();
		Region->Height = Texture->GetSizeY();

		FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

		RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
		RegionData->MipIndex = MipIndex;
		RegionData->Region = Region;
		RegionData->SrcPitch = SrcPitch;
		RegionData->SrcBpp = SrcBpp;
		RegionData->SrcData = SrcData;

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			UpdateTextureRegionsData,
			FUpdateTextureRegionsData*, RegionData, RegionData,
			{
				int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
				if (RegionData->MipIndex >= CurrentFirstMip)
				{
					RHIUpdateTexture2D(
						RegionData->Texture2DResource->GetTexture2DRHI(),
						RegionData->MipIndex - CurrentFirstMip,
						*(RegionData->Region),
						RegionData->SrcPitch,
						RegionData->SrcData
						+ RegionData->Region->SrcY * RegionData->SrcPitch
						+ RegionData->Region->SrcX * RegionData->SrcBpp
					);
				}

				delete RegionData->Region;
				delete RegionData;
			});
	}
}

// Called every frame
void UOceanTextureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (!OceanHeightTexture || !OceanNormalTexture)
		return;

	_Time += DeltaTime;

	// sum up the waves at this timestep
	long long t_start = timeGetTime();
	_Ocean->update(_Time, *_OceanContext, true, bChop, true, bJacobian, OceanScale, Choppyness);
	long long t_end = timeGetTime();
	UE_LOG(LogOceanTextureComponent, Log, TEXT("ocean update time: %d.\n"), int(t_end - t_start));

	t_start = timeGetTime();
	int size = OceanHeightTexture->GetSizeX();
	float hmax = -9999999.f;
	float hmin = 9999999.f;
	for (int y = 0; y < size; ++y)
	{
		for (int x = 0; x < size; ++x)
		{
			// height and chopness
			FLinearColor lc;
			lc.R = _OceanContext->getHF(x, y) * 0.5f + 0.5f;
			lc.G = _OceanContext->getChopX(x, y) * 0.5f + 0.5f;
			lc.B = _OceanContext->getChopY(x, y) * 0.5f + 0.5f;
			lc.A = 1.f;
			FColor c = lc.ToFColor(false);

			if (lc.R > hmax)
				hmax = lc.R;
			if (lc.R < hmin)
				hmin = lc.R;

			HeightFieldPixels[(y * size + x) * 4 + 0] = c.B;
			HeightFieldPixels[(y * size + x) * 4 + 1] = c.G;
			HeightFieldPixels[(y * size + x) * 4 + 2] = c.R;
			HeightFieldPixels[(y * size + x) * 4 + 3] = c.A;

			// normals
			//lc.R = _OceanContext->getNormalX(x, y) * 0.5f + 0.5f;
			//lc.G = _OceanContext->getNormalZ(x, y) * 0.5f + 0.5f;
			//lc.B = _OceanContext->getNormalY(x, y) * 0.5f + 0.5f;

			_OceanContext->eval2_xz(x, y);
			lc.R = _OceanContext->normal[0] * 0.5f + 0.5f;
			lc.G = _OceanContext->normal[2] * 0.5f + 0.5f;
			lc.B = _OceanContext->normal[1] * 0.5f + 0.5f;

			c = lc.ToFColor(false);
			NormalPixels[(y * size + x) * 4 + 0] = c.B;
			NormalPixels[(y * size + x) * 4 + 1] = c.G;
			NormalPixels[(y * size + x) * 4 + 2] = c.R;
			NormalPixels[(y * size + x) * 4 + 3] = c.A;
		}
	}
	t_end = timeGetTime();
	UE_LOG(LogOceanTextureComponent, Log, TEXT("hmax: %f; hmin: %f.\n"), hmax, hmin);
	UE_LOG(LogOceanTextureComponent, Log, TEXT("construct pixel time: %d.\n"), int(t_end - t_start));

	UpdateTextureRegions(OceanHeightTexture, 0, size * 4, 4, HeightFieldPixels, true);
	UpdateTextureRegions(OceanNormalTexture, 0, size * 4, 4, NormalPixels, true);
}
